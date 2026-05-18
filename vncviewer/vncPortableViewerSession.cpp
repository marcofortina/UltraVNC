// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerSession.h"

#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <cstring>

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

using uvnc::winvnc::portable::AuthOkValue;
using uvnc::winvnc::portable::EncodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::EncodeKeyEvent;
using uvnc::winvnc::portable::EncodePointerEvent;
using uvnc::winvnc::portable::IsProtocolVersionMessage;
using uvnc::winvnc::portable::ProtocolVersion38;
using uvnc::winvnc::portable::KeyEvent;
using uvnc::winvnc::portable::PointerEvent;
using uvnc::winvnc::portable::TcpSocket;

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

bool ReadServerInit(TcpSocket& socket, ViewerSessionResult& result, std::string *error)
{
    rfbServerInitMsg init;
    if (!socket.ReadExact(&init, sz_rfbServerInitMsg)) {
        SetError(error, "failed to read RFB ServerInit");
        return false;
    }

    result.width = Swap16IfLE(init.framebufferWidth);
    result.height = Swap16IfLE(init.framebufferHeight);
    result.format = init.format;
    result.format.redMax = Swap16IfLE(result.format.redMax);
    result.format.greenMax = Swap16IfLE(result.format.greenMax);
    result.format.blueMax = Swap16IfLE(result.format.blueMax);

    const CARD32 nameLength = Swap32IfLE(init.nameLength);
    result.desktopName.assign(nameLength, '\0');
    if (nameLength > 0 && !socket.ReadExact(&result.desktopName[0], nameLength)) {
        SetError(error, "failed to read RFB desktop name");
        return false;
    }
    return true;
}

bool RunHandshakeOnSocket(TcpSocket& socket, const ViewerConfig& config, ViewerSessionResult& result, std::string *error)
{
    char serverVersion[sz_rfbProtocolVersionMsg] = {};
    if (!socket.ReadExact(serverVersion, sizeof(serverVersion))) {
        SetError(error, "failed to read RFB server protocol version");
        return false;
    }
    if (!IsProtocolVersionMessage(std::string(serverVersion, sizeof(serverVersion)))) {
        SetError(error, "invalid RFB server protocol version");
        return false;
    }

    const std::string clientVersion = ProtocolVersion38();
    if (!socket.WriteAll(clientVersion.data(), clientVersion.size())) {
        SetError(error, "failed to write RFB client protocol version");
        return false;
    }

    CARD8 security[2] = {};
    if (!socket.ReadExact(security, sizeof(security))) {
        SetError(error, "failed to read RFB security types");
        return false;
    }
    if (security[0] != 1 || security[1] != rfbNoAuth) {
        SetError(error, "RFB server does not offer no-auth security");
        return false;
    }

    CARD8 selectedSecurity = rfbNoAuth;
    if (!socket.WriteAll(&selectedSecurity, sizeof(selectedSecurity))) {
        SetError(error, "failed to select RFB no-auth security");
        return false;
    }

    CARD32 authResult = 1;
    if (!socket.ReadExact(&authResult, sizeof(authResult)) || authResult != 0) {
        SetError(error, "RFB no-auth security failed");
        return false;
    }

    rfbClientInitMsg clientInit;
    std::memset(&clientInit, 0, sizeof(clientInit));
    clientInit.flags = config.Shared() ? clientInitShared : clientInitNotShare;
    if (!socket.WriteAll(&clientInit, sz_rfbClientInitMsg)) {
        SetError(error, "failed to write RFB ClientInit");
        return false;
    }

    return ReadServerInit(socket, result, error);
}

bool ReadOneRawUpdate(TcpSocket& socket, ViewerSessionResult& result, std::string *error)
{
    rfbFramebufferUpdateMsg update;
    if (!socket.ReadExact(&update, sz_rfbFramebufferUpdateMsg)) {
        SetError(error, "failed to read RFB framebuffer update header");
        return false;
    }
    if (update.type != rfbFramebufferUpdate) {
        SetError(error, "unexpected RFB framebuffer update header");
        return false;
    }
    const CARD16 rects = Swap16IfLE(update.nRects);
    if (rects == 0) {
        result.update = ViewerFramebufferUpdate();
        result.update.received = true;
        return true;
    }
    if (rects != 1) {
        SetError(error, "unsupported RFB framebuffer update rectangle count");
        return false;
    }

    rfbFramebufferUpdateRectHeader rect;
    if (!socket.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
        SetError(error, "failed to read RFB framebuffer update rectangle");
        return false;
    }

    result.update.x = Swap16IfLE(rect.r.x);
    result.update.y = Swap16IfLE(rect.r.y);
    result.update.width = Swap16IfLE(rect.r.w);
    result.update.height = Swap16IfLE(rect.r.h);
    result.update.encoding = Swap32IfLE(rect.encoding);
    if (result.update.encoding != rfbEncodingRaw) {
        SetError(error, "unsupported RFB framebuffer update encoding");
        return false;
    }

    const unsigned int bytesPerPixel = result.format.bitsPerPixel / 8;
    if (bytesPerPixel == 0 || result.update.width == 0 || result.update.height == 0) {
        SetError(error, "invalid RFB framebuffer update dimensions");
        return false;
    }
    result.update.pixels.resize(result.update.width * result.update.height * bytesPerPixel);
    if (!socket.ReadExact(result.update.pixels.data(), result.update.pixels.size())) {
        SetError(error, "failed to read RFB raw framebuffer update pixels");
        return false;
    }
    result.update.received = true;
    return true;
}

} // namespace

ViewerFramebufferUpdate::ViewerFramebufferUpdate()
    : received(false),
      x(0),
      y(0),
      width(0),
      height(0),
      encoding(0),
      pixels()
{
}

ViewerSessionResult::ViewerSessionResult()
    : width(0),
      height(0),
      format(),
      desktopName(),
      update()
{
    std::memset(&format, 0, sizeof(format));
}

bool ViewerSession::RunHandshake(const ViewerConfig& config, ViewerSessionResult& result, std::string *error) const
{
    std::string validationError;
    if (!config.Validate(&validationError)) {
        SetError(error, validationError);
        return false;
    }

    TcpSocket socket;
    if (!TcpSocket::Connect(config.Host(), config.Port(), socket)) {
        SetError(error, "failed to connect to RFB server");
        return false;
    }
    result = ViewerSessionResult();
    return RunHandshakeOnSocket(socket, config, result, error);
}

bool ViewerSession::RequestOneFramebufferUpdate(const ViewerConfig& config, ViewerSessionResult& result, std::string *error) const
{
    PersistentViewerSession session;
    if (!session.Connect(config, result, error)) {
        return false;
    }
    return session.RequestFramebufferUpdate(false, result, error);
}

PersistentViewerSession::PersistentViewerSession()
    : socket_(),
      state_()
{
}

PersistentViewerSession::~PersistentViewerSession()
{
    Disconnect();
}

bool PersistentViewerSession::Connect(const ViewerConfig& config, ViewerSessionResult& result, std::string *error)
{
    std::string validationError;
    if (!config.Validate(&validationError)) {
        SetError(error, validationError);
        return false;
    }

    Disconnect();
    if (!TcpSocket::Connect(config.Host(), config.Port(), socket_)) {
        SetError(error, "failed to connect to RFB server");
        return false;
    }
    state_ = ViewerSessionResult();
    if (!RunHandshakeOnSocket(socket_, config, state_, error)) {
        Disconnect();
        return false;
    }
    result = state_;
    return true;
}

bool PersistentViewerSession::Connected() const
{
    return socket_.Valid();
}

void PersistentViewerSession::Disconnect()
{
    socket_.Close();
    state_ = ViewerSessionResult();
}

bool PersistentViewerSession::RequestFramebufferUpdate(bool incremental, ViewerSessionResult& result, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }

    uvnc::winvnc::portable::FramebufferUpdateRequest request;
    request.incremental = incremental;
    request.x = 0;
    request.y = 0;
    request.width = state_.width;
    request.height = state_.height;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    if (!socket_.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg)) {
        SetError(error, "failed to write RFB framebuffer update request");
        Disconnect();
        return false;
    }
    state_.update = ViewerFramebufferUpdate();
    if (!ReadOneRawUpdate(socket_, state_, error)) {
        Disconnect();
        return false;
    }
    result = state_;
    return true;
}

bool PersistentViewerSession::SendKeyEvent(CARD32 keysym, bool down, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const KeyEvent event{down, keysym};
    const rfbKeyEventMsg wire = EncodeKeyEvent(event);
    if (!socket_.WriteAll(&wire, sz_rfbKeyEventMsg)) {
        SetError(error, "failed to write RFB key event");
        Disconnect();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool PersistentViewerSession::SendPointerEvent(CARD8 buttonMask, unsigned int x, unsigned int y, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const PointerEvent event{buttonMask, x, y};
    const rfbPointerEventMsg wire = EncodePointerEvent(event);
    if (!socket_.WriteAll(&wire, sz_rfbPointerEventMsg)) {
        SetError(error, "failed to write RFB pointer event");
        Disconnect();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
