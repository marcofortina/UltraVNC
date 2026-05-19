// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

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
    const std::string name = "server-messages-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(2));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(2));
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
    if (!payload.empty() && !socket.ReadExact(payload.data(), payload.size())) {
        return false;
    }
    rfbClientCutTextMsg cutText;
    if (!socket.ReadExact(&cutText, sz_rfbClientCutTextMsg)) {
        return false;
    }
    const int32_t length = static_cast<int32_t>(Swap32IfLE(cutText.length));
    if (cutText.type != rfbClientCutText || length >= 0) {
        return false;
    }
    std::vector<CARD8> caps(static_cast<std::size_t>(-length));
    return caps.empty() || socket.ReadExact(caps.data(), caps.size());
}

bool WriteServerCutText(TcpSocket& socket, const std::string& text)
{
    rfbServerCutTextMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbServerCutText;
    message.length = Swap32IfLE(static_cast<CARD32>(text.size()));
    return socket.WriteAll(&message, sz_rfbServerCutTextMsg) && socket.WriteAll(text.data(), text.size());
}

bool WriteRawUpdate(TcpSocket& socket)
{
    rfbFramebufferUpdateRequestMsg request;
    if (!socket.ReadExact(&request, sz_rfbFramebufferUpdateRequestMsg)) return false;
    FramebufferUpdateRequest decoded;
    if (!DecodeFramebufferUpdateRequest(request, decoded)) return false;

    const CARD8 bell = rfbBell;
    if (!socket.WriteAll(&bell, sizeof(bell))) return false;
    if (!WriteServerCutText(socket, "server clipboard")) return false;

    rfbFramebufferUpdateMsg update;
    update.type = rfbFramebufferUpdate;
    update.pad = 0;
    update.nRects = Swap16IfLE(static_cast<CARD16>(1));
    rfbFramebufferUpdateRectHeader rect;
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = Swap16IfLE(static_cast<CARD16>(2));
    rect.r.h = Swap16IfLE(static_cast<CARD16>(2));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(rfbEncodingRaw));
    std::vector<CARD8> pixels(2 * 2 * 4, 0x55);
    return socket.WriteAll(&update, sz_rfbFramebufferUpdateMsg) &&
           socket.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader) &&
           socket.WriteAll(pixels.data(), pixels.size());
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
        serverOk = listener.Accept(client) && RunNoAuthHandshake(client) && WriteRawUpdate(client);
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    assert(session.Connect(config, result, &error));
    assert(session.RequestFramebufferUpdate(false, result, &error));
    assert(result.bellCount == 1);
    assert(result.serverCutText == "server clipboard");
    assert(result.update.received);
    assert(result.update.width == 2);
    assert(result.update.height == 2);
    assert(result.update.pixels.size() == 16);
    session.Disconnect();

    server.join();
    assert(serverOk);
    return 0;
}
