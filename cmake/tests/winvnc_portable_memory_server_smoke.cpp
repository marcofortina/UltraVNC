// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"

#include <iostream>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(160, 90);
    config.SetDesktopName("memory-server-smoke");

    MemoryServer server;
    if (!server.Start(config) || !server.Running() || server.Port() == 0) {
        std::cerr << "memory server did not start\n";
        return 1;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOne();
    });

    TcpSocket client;
    if (!TcpSocket::Connect("127.0.0.1", server.Port(), client)) {
        std::cerr << "connect failed\n";
        server.Stop();
        worker.join();
        return 1;
    }

    char version[sz_rfbProtocolVersionMsg] = {};
    client.ReadExact(version, sizeof(version));
    const std::string clientVersion = ProtocolVersion38();
    client.WriteAll(clientVersion.data(), clientVersion.size());
    CARD8 security[2] = {};
    client.ReadExact(security, sizeof(security));
    CARD8 selected = rfbNoAuth;
    client.WriteAll(&selected, sizeof(selected));
    CARD32 auth = 1;
    client.ReadExact(&auth, sizeof(auth));
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    client.WriteAll(&init, sz_rfbClientInitMsg);
    rfbServerInitMsg serverInit;
    client.ReadExact(&serverInit, sz_rfbServerInitMsg);
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    client.ReadExact(&name[0], name.size());

    worker.join();
    server.Stop();

    if (!serverOk || Swap16IfLE(serverInit.framebufferWidth) != 160 || Swap16IfLE(serverInit.framebufferHeight) != 90 || name != "memory-server-smoke") {
        std::cerr << "memory server handshake failed\n";
        return 1;
    }
    return 0;
}
