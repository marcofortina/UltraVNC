// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableCursor.h"
#include "vncPortableExtendedClipboard.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"
#include "vncPortableViewerConfig.h"
#include "vncPortableViewerSession.h"

#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace uvnc::vncviewer::portable;
using namespace uvnc::winvnc::portable;

namespace {

rfbPixelFormat TestPixelFormat()
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.trueColour = 1;
    format.redMax = Swap16IfLE(static_cast<CARD16>(255));
    format.greenMax = Swap16IfLE(static_cast<CARD16>(255));
    format.blueMax = Swap16IfLE(static_cast<CARD16>(255));
    format.redShift = 16;
    format.greenShift = 8;
    return format;
}

bool WriteServerInit(TcpSocket& socket)
{
    const std::string name = "viewer-pseudo-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(4));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(4));
    init.format = TestPixelFormat();
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));
    return socket.WriteAll(&init, sz_rfbServerInitMsg) && socket.WriteAll(name.data(), name.size());
}

bool RunNoAuthHandshake(TcpSocket& socket)
{
    const std::string version = ProtocolVersion38();
    if (!socket.WriteAll(version.data(), version.size())) return false;
    char clientVersion[sz_rfbProtocolVersionMsg] = {};
    if (!socket.ReadExact(clientVersion, sizeof(clientVersion))) return false;
    CARD8 security[2] = {1, rfbNoAuth};
    if (!socket.WriteAll(security, sizeof(security))) return false;
    CARD8 selected = 0;
    if (!socket.ReadExact(&selected, sizeof(selected)) || selected != rfbNoAuth) return false;
    CARD32 ok = Swap32IfLE(static_cast<CARD32>(rfbVncAuthOK));
    if (!socket.WriteAll(&ok, sizeof(ok))) return false;
    rfbClientInitMsg clientInit;
    if (!socket.ReadExact(&clientInit, sz_rfbClientInitMsg)) return false;
    if (!WriteServerInit(socket)) return false;
    rfbSetEncodingsMsg encodings;
    if (!socket.ReadExact(&encodings, sz_rfbSetEncodingsMsg)) return false;
    unsigned int count = 0;
    if (!DecodeSetEncodingsHeader(encodings, count)) return false;
    std::vector<CARD8> payload(count * sizeof(CARD32));
    if (!payload.empty() && !socket.ReadExact(payload.data(), payload.size())) return false;
    rfbClientCutTextMsg capsHeader;
    if (!socket.ReadExact(&capsHeader, sz_rfbClientCutTextMsg)) return false;
    const int32_t length = static_cast<int32_t>(Swap32IfLE(capsHeader.length));
    if (capsHeader.type != rfbClientCutText || length >= 0) return false;
    std::vector<CARD8> caps(static_cast<std::size_t>(-length));
    return caps.empty() || socket.ReadExact(caps.data(), caps.size());
}

bool WriteExtendedClipboard(TcpSocket& socket)
{
    const std::vector<CARD8> payload = EncodeExtendedClipboardProvideText("extended clipboard from server");
    const std::vector<CARD8> message = EncodeExtendedServerCutText(payload);
    return socket.WriteAll(message.data(), message.size());
}

bool WritePointerPos(TcpSocket& socket)
{
    rfbFramebufferUpdateRequestMsg request;
    if (!socket.ReadExact(&request, sz_rfbFramebufferUpdateRequestMsg)) return false;
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(static_cast<CARD16>(1));
    rfbFramebufferUpdateRectHeader rect;
    std::memset(&rect, 0, sizeof(rect));
    rect.r.x = Swap16IfLE(static_cast<CARD16>(3));
    rect.r.y = Swap16IfLE(static_cast<CARD16>(2));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(rfbEncodingPointerPos));
    return socket.WriteAll(&update, sz_rfbFramebufferUpdateMsg) && socket.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader);
}

bool WriteRichCursor(TcpSocket& socket)
{
    CursorShape shape = DefaultArrowCursorShape();
    shape.width = 2;
    shape.height = 2;
    shape.hotspotX = 1;
    shape.hotspotY = 1;
    shape.bgra.assign(2 * 2 * 4, 0xff);
    const std::vector<CARD8> message = EncodeRichCursorShapeUpdate(shape);
    return socket.WriteAll(message.data(), message.size());
}

bool WriteLastRect(TcpSocket& socket)
{
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(static_cast<CARD16>(1));
    rfbFramebufferUpdateRectHeader rect;
    std::memset(&rect, 0, sizeof(rect));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(rfbEncodingLastRect));
    return socket.WriteAll(&update, sz_rfbFramebufferUpdateMsg) && socket.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader);
}

} // namespace

int main()
{
    TcpListener listener;
    assert(listener.Listen("127.0.0.1", 0));
    const unsigned short port = listener.Port();

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket client;
        serverOk = listener.Accept(client) &&
                   RunNoAuthHandshake(client) &&
                   WriteExtendedClipboard(client) &&
                   WritePointerPos(client) &&
                   WriteRichCursor(client) &&
                   WriteLastRect(client);
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    assert(session.Connect(config, result, &error));
    assert(session.RequestFramebufferUpdate(false, result, &error));
    assert(result.extendedClipboardReceived);
    assert(result.serverCutText == "extended clipboard from server");
    assert(result.pointerPositionReceived);
    assert(result.pointerX == 3);
    assert(result.pointerY == 2);
    assert(result.cursorShape.received);
    assert(result.cursorShape.width == 2);
    assert(result.cursorShape.height == 2);
    assert(result.cursorShape.hotspotX == 1);
    assert(result.cursorShape.hotspotY == 1);
    assert(!result.rectangles.empty());
    assert(result.rectangles.back().encoding == rfbEncodingLastRect);
    session.Disconnect();

    server.join();
    assert(serverOk);
    return 0;
}
