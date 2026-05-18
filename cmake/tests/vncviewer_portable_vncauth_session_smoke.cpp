// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfb.h"
#include "vncPortableTcp.h"
#include "vncPortableViewerConfig.h"
#include "vncPortableViewerSession.h"
#include "vncPortableVncAuth.h"

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

bool SendServerInit(TcpSocket& client)
{
    const std::string name = "vncauth-portable-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(4));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(3));
    init.format = TestPixelFormat();
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));
    return client.WriteAll(&init, sz_rfbServerInitMsg) && client.WriteAll(name.data(), name.size());
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
        CARD8 security[2] = {1, rfbVncAuth};
        if (!client.WriteAll(security, sizeof(security))) return;
        CARD8 selected = 0;
        if (!client.ReadExact(&selected, sizeof(selected)) || selected != rfbVncAuth) return;
        std::vector<unsigned char> challenge(16);
        for (unsigned int i = 0; i < challenge.size(); ++i) {
            challenge[i] = static_cast<unsigned char>(i + 1);
        }
        if (!client.WriteAll(challenge.data(), challenge.size())) return;
        std::vector<unsigned char> expected;
        std::string error;
        if (!EncryptVncAuthChallenge(challenge, "secret", expected, &error)) return;
        std::vector<unsigned char> received(expected.size());
        if (!client.ReadExact(received.data(), received.size())) return;
        if (received != expected) return;
        CARD32 ok = Swap32IfLE(static_cast<CARD32>(rfbVncAuthOK));
        if (!client.WriteAll(&ok, sizeof(ok))) return;
        rfbClientInitMsg clientInit;
        if (!client.ReadExact(&clientInit, sz_rfbClientInitMsg)) return;
        if (!SendServerInit(client)) return;
        serverOk = true;
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    config.SetPassword("secret");

    ViewerSessionResult result;
    std::string error;
    assert(ViewerSession().RunHandshake(config, result, &error));
    assert(error.empty());
    assert(result.width == 4);
    assert(result.height == 3);
    assert(result.desktopName == "vncauth-portable-test");

    server.join();
    assert(serverOk);
    return 0;
}
