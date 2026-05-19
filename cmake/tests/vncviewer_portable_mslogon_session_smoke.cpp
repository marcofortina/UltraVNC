// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMsLogon.h"
#include "vncPortableRfb.h"
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
    format.blueShift = 0;
    return format;
}

void WriteU64BE(unsigned long long value, CARD8 *out)
{
    for (int i = 7; i >= 0; --i) {
        out[i] = static_cast<CARD8>(value & 0xffULL);
        value >>= 8;
    }
}

bool SendMsLogonIIExchange(TcpSocket& client)
{
    CARD8 bytes[kMsLogonIIExchangeBytes] = {};
    WriteU64BE(5, bytes);
    WriteU64BE(23, bytes + 8);
    WriteU64BE(8, bytes + 16);
    return client.WriteAll(bytes, sizeof(bytes));
}

bool SendServerInit(TcpSocket& client)
{
    const std::string name = "mslogon-portable-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(4));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(3));
    init.format = TestPixelFormat();
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));
    return client.WriteAll(&init, sz_rfbServerInitMsg) && client.WriteAll(name.data(), name.size());
}

bool ReadViewerEncodingsAndCaps(TcpSocket& client)
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

} // namespace

int main()
{
    TcpListener listener;
    assert(listener.Listen("127.0.0.1", 0));
    const unsigned short port = listener.Port();

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket client;
        if (!listener.Accept(client)) return;
        const std::string version = ProtocolVersion38();
        if (!client.WriteAll(version.data(), version.size())) return;
        char clientVersion[sz_rfbProtocolVersionMsg] = {};
        if (!client.ReadExact(clientVersion, sizeof(clientVersion))) return;
        CARD8 security[2] = {1, rfbUltraVNC_MsLogonIIAuth};
        if (!client.WriteAll(security, sizeof(security))) return;
        CARD8 selected = 0;
        if (!client.ReadExact(&selected, sizeof(selected)) || selected != rfbUltraVNC_MsLogonIIAuth) return;
        if (!SendMsLogonIIExchange(client)) return;
        std::vector<CARD8> response(kMsLogonIIResponseBytes);
        if (!client.ReadExact(response.data(), response.size())) return;
        CARD32 ok = Swap32IfLE(static_cast<CARD32>(rfbVncAuthOK));
        if (!client.WriteAll(&ok, sizeof(ok))) return;
        rfbClientInitMsg clientInit;
        if (!client.ReadExact(&clientInit, sz_rfbClientInitMsg)) return;
        if (!SendServerInit(client)) return;
        if (!ReadViewerEncodingsAndCaps(client)) return;
        serverOk = true;
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    config.SetSecurityExtension(ViewerSecurityExtensionMode::MsLogon);
    config.SetUsername("LAB\\alice");
    config.SetPassword("secret");

    ViewerSessionResult result;
    std::string error;
    assert(ViewerSession().RunHandshake(config, result, &error));
    assert(error.empty());
    assert(result.width == 4);
    assert(result.height == 3);
    assert(result.desktopName == "mslogon-portable-test");

    server.join();
    assert(serverOk);
    return 0;
}
