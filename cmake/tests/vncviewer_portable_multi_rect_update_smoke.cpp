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

bool SendServerInit(TcpSocket& client)
{
    const std::string name = "multi-rect-update-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(6));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(4));
    init.format = TestPixelFormat();
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));
    return client.WriteAll(&init, sz_rfbServerInitMsg) && client.WriteAll(name.data(), name.size());
}

bool RunMinimalNoAuthHandshake(TcpSocket& client)
{
    const std::string version = ProtocolVersion38();
    if (!client.WriteAll(version.data(), version.size())) return false;
    char clientVersion[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(clientVersion, sizeof(clientVersion))) return false;
    CARD8 security[2] = {1, rfbNoAuth};
    if (!client.WriteAll(security, sizeof(security))) return false;
    CARD8 selected = 0;
    if (!client.ReadExact(&selected, sizeof(selected)) || selected != rfbNoAuth) return false;
    CARD32 ok = Swap32IfLE(static_cast<CARD32>(rfbVncAuthOK));
    if (!client.WriteAll(&ok, sizeof(ok))) return false;
    rfbClientInitMsg clientInit;
    if (!client.ReadExact(&clientInit, sz_rfbClientInitMsg)) return false;
    return SendServerInit(client);
}

bool ReadSetEncodings(TcpSocket& client)
{
    rfbSetEncodingsMsg header;
    if (!client.ReadExact(&header, sz_rfbSetEncodingsMsg)) return false;
    unsigned int count = 0;
    if (!DecodeSetEncodingsHeader(header, count)) return false;
    std::vector<CARD8> payload(count * sizeof(CARD32));
    if (!payload.empty() && !client.ReadExact(payload.data(), payload.size())) return false;
    rfbClientCutTextMsg capsHeader;
    if (!client.ReadExact(&capsHeader, sz_rfbClientCutTextMsg)) return false;
    const int32_t length = static_cast<int32_t>(Swap32IfLE(capsHeader.length));
    if (capsHeader.type != rfbClientCutText || length >= 0) return false;
    std::vector<CARD8> caps(static_cast<std::size_t>(-length));
    return caps.empty() || client.ReadExact(caps.data(), caps.size());
}

bool ReadUpdateRequest(TcpSocket& client)
{
    rfbFramebufferUpdateRequestMsg request;
    if (!client.ReadExact(&request, sz_rfbFramebufferUpdateRequestMsg)) return false;
    FramebufferUpdateRequest decoded;
    return DecodeFramebufferUpdateRequest(request, decoded);
}

bool SendRawRect(TcpSocket& client, unsigned int x, unsigned int y, unsigned int w, unsigned int h, CARD8 value)
{
    rfbFramebufferUpdateRectHeader rect;
    rect.r.x = Swap16IfLE(static_cast<CARD16>(x));
    rect.r.y = Swap16IfLE(static_cast<CARD16>(y));
    rect.r.w = Swap16IfLE(static_cast<CARD16>(w));
    rect.r.h = Swap16IfLE(static_cast<CARD16>(h));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(rfbEncodingRaw));
    std::vector<CARD8> pixels(static_cast<std::size_t>(w) * h * 4, value);
    return client.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader) &&
           client.WriteAll(pixels.data(), pixels.size());
}

bool SendMultiRectUpdate(TcpSocket& client)
{
    if (!ReadUpdateRequest(client)) return false;
    rfbFramebufferUpdateMsg update;
    update.type = rfbFramebufferUpdate;
    update.pad = 0;
    update.nRects = Swap16IfLE(static_cast<CARD16>(2));
    return client.WriteAll(&update, sz_rfbFramebufferUpdateMsg) &&
           SendRawRect(client, 0, 0, 3, 4, 0x11) &&
           SendRawRect(client, 3, 0, 3, 4, 0x22);
}

std::size_t PixelOffset(unsigned int x, unsigned int y)
{
    return (static_cast<std::size_t>(y) * 6 + x) * 4;
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
                   RunMinimalNoAuthHandshake(client) &&
                   ReadSetEncodings(client) &&
                   SendMultiRectUpdate(client);
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    assert(session.Connect(config, result, &error));
    assert(session.RequestFramebufferUpdate(false, result, &error));
    assert(result.update.received);
    assert(result.rectangles.size() == 2);
    assert(result.update.encoding == rfbEncodingRaw);
    assert(result.update.width == 6);
    assert(result.update.height == 4);
    assert(result.update.pixels.size() == 6 * 4 * 4);
    assert(result.update.pixels[PixelOffset(0, 0)] == 0x11);
    assert(result.update.pixels[PixelOffset(2, 3)] == 0x11);
    assert(result.update.pixels[PixelOffset(3, 0)] == 0x22);
    assert(result.update.pixels[PixelOffset(5, 3)] == 0x22);
    session.Disconnect();

    server.join();
    assert(serverOk);
    return 0;
}
