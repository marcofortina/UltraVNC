// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"
#include "vncPortableRfb.h"

#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    int fds[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        std::cerr << "socketpair failed\n";
        return 1;
    }

    ServerConfig config;
    config.SetSize(320, 200);
    config.SetDesktopName("session-smoke");

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket serverSocket(fds[0]);
        serverOk = RfbServerSession().RunHandshake(serverSocket, config);
    });

    TcpSocket clientSocket(fds[1]);
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!clientSocket.ReadExact(version, sizeof(version))) {
        std::cerr << "missing server version\n";
        server.join();
        return 1;
    }
    if (std::string(version, sizeof(version)) != ProtocolVersion38()) {
        std::cerr << "unexpected server version\n";
        server.join();
        return 1;
    }
    const std::string clientVersion = ProtocolVersion38();
    clientSocket.WriteAll(clientVersion.data(), clientVersion.size());

    CARD8 security[2] = {};
    clientSocket.ReadExact(security, sizeof(security));
    if (security[0] != 1 || security[1] != rfbNoAuth) {
        std::cerr << "unexpected security list\n";
        server.join();
        return 1;
    }
    CARD8 selected = rfbNoAuth;
    clientSocket.WriteAll(&selected, sizeof(selected));

    CARD32 auth = 1;
    clientSocket.ReadExact(&auth, sizeof(auth));
    if (Swap32IfLE(auth) != 0) {
        std::cerr << "auth was not OK\n";
        server.join();
        return 1;
    }

    rfbClientInitMsg init;
    init.flags = clientInitShared;
    clientSocket.WriteAll(&init, sz_rfbClientInitMsg);

    rfbServerInitMsg serverInit;
    clientSocket.ReadExact(&serverInit, sz_rfbServerInitMsg);
    if (Swap16IfLE(serverInit.framebufferWidth) != 320 || Swap16IfLE(serverInit.framebufferHeight) != 200) {
        std::cerr << "server init dimensions mismatch\n";
        server.join();
        return 1;
    }
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    clientSocket.ReadExact(&name[0], name.size());
    server.join();
    if (!serverOk || name != "session-smoke") {
        std::cerr << "session handshake failed\n";
        return 1;
    }
    return 0;
}
