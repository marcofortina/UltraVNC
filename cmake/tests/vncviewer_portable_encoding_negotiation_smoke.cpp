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
    const std::string name = "encoding-negotiation-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(64));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(48));
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

bool ReadSetEncodings(TcpSocket& client, std::vector<CARD32>& encodings)
{
    rfbSetEncodingsMsg header;
    if (!client.ReadExact(&header, sz_rfbSetEncodingsMsg)) return false;
    unsigned int count = 0;
    if (!DecodeSetEncodingsHeader(header, count)) return false;
    std::vector<CARD8> payload(count * sizeof(CARD32));
    if (!payload.empty() && !client.ReadExact(payload.data(), payload.size())) return false;
    encodings = DecodeSetEncodingsPayload(payload);
    return true;
}

bool SendCopyRectUpdate(TcpSocket& client)
{
    rfbFramebufferUpdateRequestMsg request;
    if (!client.ReadExact(&request, sz_rfbFramebufferUpdateRequestMsg)) return false;
    FramebufferUpdateRequest decoded;
    if (!DecodeFramebufferUpdateRequest(request, decoded)) return false;

    rfbFramebufferUpdateMsg update;
    update.type = rfbFramebufferUpdate;
    update.pad = 0;
    update.nRects = Swap16IfLE(static_cast<CARD16>(1));
    rfbFramebufferUpdateRectHeader rect;
    rect.r.x = Swap16IfLE(static_cast<CARD16>(4));
    rect.r.y = Swap16IfLE(static_cast<CARD16>(5));
    rect.r.w = Swap16IfLE(static_cast<CARD16>(8));
    rect.r.h = Swap16IfLE(static_cast<CARD16>(9));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(rfbEncodingCopyRect));
    rfbCopyRect copy;
    copy.srcX = Swap16IfLE(static_cast<CARD16>(1));
    copy.srcY = Swap16IfLE(static_cast<CARD16>(2));
    return client.WriteAll(&update, sz_rfbFramebufferUpdateMsg) &&
           client.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader) &&
           client.WriteAll(&copy, sz_rfbCopyRect);
}

} // namespace

int main()
{
    TcpListener listener;
    assert(listener.Listen("127.0.0.1", 0));
    const unsigned short port = listener.Port();

    bool serverOk = false;
    std::vector<CARD32> encodings;
    std::thread server([&]() {
        TcpSocket client;
        serverOk = listener.Accept(client) &&
                   RunMinimalNoAuthHandshake(client) &&
                   ReadSetEncodings(client, encodings) &&
                   SendCopyRectUpdate(client);
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    std::vector<unsigned int> legacyEncodings;
    legacyEncodings.push_back(rfbEncodingRaw);
    legacyEncodings.push_back(rfbEncodingCopyRect);
    legacyEncodings.push_back(rfbEncodingHextile);
    legacyEncodings.push_back(rfbEncodingZlib);
    legacyEncodings.push_back(rfbEncodingZRLE);
    legacyEncodings.push_back(rfbEncodingTight);
    legacyEncodings.push_back(rfbEncodingRRE);
    legacyEncodings.push_back(rfbEncodingCoRRE);
    legacyEncodings.push_back(rfbEncodingNewFBSize);
    config.SetEncodings(legacyEncodings);
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    assert(session.Connect(config, result, &error));
    assert(session.RequestFramebufferUpdate(false, result, &error));
    assert(result.update.received);
    assert(result.update.encoding == rfbEncodingRaw);
    assert(result.update.width == 64);
    assert(result.update.height == 48);
    assert(result.rectangles.size() == 1);
    assert(result.rectangles[0].encoding == rfbEncodingCopyRect);
    assert(result.rectangles[0].x == 4);
    assert(result.rectangles[0].y == 5);
    assert(result.rectangles[0].width == 8);
    assert(result.rectangles[0].height == 9);
    assert(result.rectangles[0].sourceX == 1);
    assert(result.rectangles[0].sourceY == 2);
    session.Disconnect();

    server.join();
    assert(serverOk);
    assert(encodings.size() >= 2);
    assert(encodings[0] == rfbEncodingRaw);
    assert(encodings[1] == rfbEncodingCopyRect);
    return 0;
}
