// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerSession.h"

#include "vncPortableVncAuth.h"

#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

using uvnc::winvnc::portable::AuthOkValue;
using uvnc::winvnc::portable::EncodeClientCutText;
using uvnc::winvnc::portable::EncodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::EncodeKeyEvent;
using uvnc::winvnc::portable::EncodePointerEvent;
using uvnc::winvnc::portable::EncodeSetEncodings;
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

    CARD8 securityCount = 0;
    if (!socket.ReadExact(&securityCount, sizeof(securityCount))) {
        SetError(error, "failed to read RFB security type count");
        return false;
    }
    if (securityCount == 0) {
        SetError(error, "RFB server reported no security types");
        return false;
    }
    std::vector<CARD8> securityTypes(securityCount);
    if (!socket.ReadExact(securityTypes.data(), securityTypes.size())) {
        SetError(error, "failed to read RFB security types");
        return false;
    }

    bool offersNoAuth = false;
    bool offersVncAuth = false;
    for (std::size_t i = 0; i < securityTypes.size(); ++i) {
        offersNoAuth = offersNoAuth || securityTypes[i] == rfbNoAuth;
        offersVncAuth = offersVncAuth || securityTypes[i] == rfbVncAuth;
    }

    CARD8 selectedSecurity = 0;
    if (!config.Password().empty() && offersVncAuth) {
        selectedSecurity = rfbVncAuth;
    } else if (offersNoAuth) {
        selectedSecurity = rfbNoAuth;
    } else if (offersVncAuth) {
        SetError(error, "RFB server requires VNCAuth but no password was provided");
        return false;
    } else {
        SetError(error, "RFB server does not offer a supported security type");
        return false;
    }

    if (!socket.WriteAll(&selectedSecurity, sizeof(selectedSecurity))) {
        SetError(error, "failed to select RFB security type");
        return false;
    }

    if (selectedSecurity == rfbVncAuth) {
        std::vector<unsigned char> challenge(16);
        if (!socket.ReadExact(challenge.data(), challenge.size())) {
            SetError(error, "failed to read RFB VNCAuth challenge");
            return false;
        }
        std::vector<unsigned char> response;
        if (!EncryptVncAuthChallenge(challenge, config.Password(), response, error)) {
            return false;
        }
        if (!socket.WriteAll(response.data(), response.size())) {
            SetError(error, "failed to write RFB VNCAuth response");
            return false;
        }
    }

    CARD32 authResult = 1;
    if (!socket.ReadExact(&authResult, sizeof(authResult)) || Swap32IfLE(authResult) != rfbVncAuthOK) {
        SetError(error, selectedSecurity == rfbVncAuth ? "RFB VNCAuth security failed" : "RFB no-auth security failed");
        return false;
    }

    rfbClientInitMsg clientInit;
    std::memset(&clientInit, 0, sizeof(clientInit));
    clientInit.flags = config.Shared() ? clientInitShared : clientInitNotShare;
    if (!socket.WriteAll(&clientInit, sz_rfbClientInitMsg)) {
        SetError(error, "failed to write RFB ClientInit");
        return false;
    }

    if (!ReadServerInit(socket, result, error)) {
        return false;
    }
    std::vector<CARD32> encodings;
    for (std::size_t i = 0; i < config.Encodings().size(); ++i) {
        encodings.push_back(static_cast<CARD32>(config.Encodings()[i]));
    }
    const std::vector<CARD8> setEncodings = EncodeSetEncodings(encodings);
    if (!socket.WriteAll(setEncodings.data(), setEncodings.size())) {
        SetError(error, "failed to write RFB SetEncodings");
        return false;
    }
    return true;
}

unsigned int BytesPerPixel(const rfbPixelFormat& format)
{
    return format.bitsPerPixel / 8;
}

bool EnsureFramebufferStorage(ViewerSessionResult& result, std::vector<CARD8>& framebuffer, std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || result.width == 0 || result.height == 0) {
        SetError(error, "invalid RFB framebuffer dimensions");
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(result.width) * result.height * bytesPerPixel;
    if (framebuffer.size() != expected) {
        framebuffer.assign(expected, 0);
    }
    return true;
}

bool ReadRawRectPayload(TcpSocket& socket,
                        ViewerSessionResult& result,
                        std::vector<CARD8>& framebuffer,
                        ViewerFramebufferRect& rectangle,
                        std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB raw rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB raw rectangle is outside framebuffer bounds");
        return false;
    }
    rectangle.pixels.resize(static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel);
    if (!socket.ReadExact(rectangle.pixels.data(), rectangle.pixels.size())) {
        SetError(error, "failed to read RFB raw framebuffer update pixels");
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    for (unsigned int row = 0; row < rectangle.height; ++row) {
        const std::size_t src = static_cast<std::size_t>(row) * rectangle.width * bytesPerPixel;
        const std::size_t dst = (static_cast<std::size_t>(rectangle.y + row) * result.width + rectangle.x) * bytesPerPixel;
        std::copy(rectangle.pixels.begin() + src,
                  rectangle.pixels.begin() + src + static_cast<std::size_t>(rectangle.width) * bytesPerPixel,
                  framebuffer.begin() + dst);
    }
    return true;
}

bool ApplyCopyRectPayload(TcpSocket& socket,
                          ViewerSessionResult& result,
                          std::vector<CARD8>& framebuffer,
                          ViewerFramebufferRect& rectangle,
                          std::string *error)
{
    rfbCopyRect copyRect;
    if (!socket.ReadExact(&copyRect, sz_rfbCopyRect)) {
        SetError(error, "failed to read RFB CopyRect payload");
        return false;
    }
    rectangle.sourceX = Swap16IfLE(copyRect.srcX);
    rectangle.sourceY = Swap16IfLE(copyRect.srcY);
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB CopyRect dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height ||
        rectangle.sourceX + rectangle.width > result.width || rectangle.sourceY + rectangle.height > result.height) {
        SetError(error, "RFB CopyRect rectangle is outside framebuffer bounds");
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }

    std::vector<CARD8> copy(static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel);
    for (unsigned int row = 0; row < rectangle.height; ++row) {
        const std::size_t src = (static_cast<std::size_t>(rectangle.sourceY + row) * result.width + rectangle.sourceX) * bytesPerPixel;
        const std::size_t dst = static_cast<std::size_t>(row) * rectangle.width * bytesPerPixel;
        std::copy(framebuffer.begin() + src,
                  framebuffer.begin() + src + static_cast<std::size_t>(rectangle.width) * bytesPerPixel,
                  copy.begin() + dst);
    }
    for (unsigned int row = 0; row < rectangle.height; ++row) {
        const std::size_t src = static_cast<std::size_t>(row) * rectangle.width * bytesPerPixel;
        const std::size_t dst = (static_cast<std::size_t>(rectangle.y + row) * result.width + rectangle.x) * bytesPerPixel;
        std::copy(copy.begin() + src,
                  copy.begin() + src + static_cast<std::size_t>(rectangle.width) * bytesPerPixel,
                  framebuffer.begin() + dst);
    }
    return true;
}

bool PublishCompositedFramebuffer(ViewerSessionResult& result,
                                  const std::vector<CARD8>& framebuffer,
                                  std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    const std::size_t expected = static_cast<std::size_t>(result.width) * result.height * bytesPerPixel;
    if (bytesPerPixel == 0 || framebuffer.size() != expected) {
        SetError(error, "invalid composed RFB framebuffer size");
        return false;
    }
    result.update = ViewerFramebufferUpdate();
    result.update.received = true;
    result.update.x = 0;
    result.update.y = 0;
    result.update.width = result.width;
    result.update.height = result.height;
    result.update.encoding = rfbEncodingRaw;
    result.update.pixels = framebuffer;
    return true;
}

bool ReadFramebufferUpdate(TcpSocket& socket,
                           ViewerSessionResult& result,
                           std::vector<CARD8>& framebuffer,
                           std::string *error)
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

    result.rectangles.clear();
    const CARD16 rects = Swap16IfLE(update.nRects);
    if (rects == 0) {
        result.update = ViewerFramebufferUpdate();
        result.update.received = true;
        return true;
    }

    for (CARD16 i = 0; i < rects; ++i) {
        rfbFramebufferUpdateRectHeader rect;
        if (!socket.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
            SetError(error, "failed to read RFB framebuffer update rectangle");
            return false;
        }

        ViewerFramebufferRect rectangle;
        rectangle.x = Swap16IfLE(rect.r.x);
        rectangle.y = Swap16IfLE(rect.r.y);
        rectangle.width = Swap16IfLE(rect.r.w);
        rectangle.height = Swap16IfLE(rect.r.h);
        rectangle.encoding = Swap32IfLE(rect.encoding);

        if (rectangle.encoding == rfbEncodingRaw) {
            if (!ReadRawRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingCopyRect) {
            if (!ApplyCopyRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingNewFBSize) {
            result.width = rectangle.width;
            result.height = rectangle.height;
            framebuffer.clear();
        } else {
            SetError(error, "unsupported RFB framebuffer update encoding");
            return false;
        }
        result.rectangles.push_back(rectangle);
    }

    if (!framebuffer.empty()) {
        return PublishCompositedFramebuffer(result, framebuffer, error);
    }
    result.update = ViewerFramebufferUpdate();
    result.update.received = true;
    return true;
}


} // namespace

ViewerFramebufferRect::ViewerFramebufferRect()
    : x(0),
      y(0),
      width(0),
      height(0),
      sourceX(0),
      sourceY(0),
      encoding(0),
      pixels()
{
}

ViewerFramebufferUpdate::ViewerFramebufferUpdate()
    : received(false),
      x(0),
      y(0),
      width(0),
      height(0),
      sourceX(0),
      sourceY(0),
      encoding(0),
      pixels()
{
}

ViewerSessionResult::ViewerSessionResult()
    : width(0),
      height(0),
      format(),
      desktopName(),
      update(),
      rectangles()
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
    config_ = config;
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
    framebuffer_.clear();
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
    if (!ReadFramebufferUpdate(socket_, state_, framebuffer_, error)) {
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


bool PersistentViewerSession::SendClientCutText(const std::string& text, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const std::vector<CARD8> bytes = EncodeClientCutText(text);
    if (!socket_.WriteAll(bytes.data(), bytes.size())) {
        SetError(error, "failed to write RFB client cut text");
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
