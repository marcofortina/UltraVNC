// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfb.h"
#include "vncPortableRfbSession.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <thread>

using namespace uvnc::winvnc::portable;

int main()
{
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    ServerConfig config;
    bool serverResult = true;
    std::thread server([&]() {
        TcpSocket socket(fds[0]);
        serverResult = RfbServerSession().RunHandshake(socket, config);
    });

    TcpSocket client(fds[1]);
    char version[sz_rfbProtocolVersionMsg] = {};
    assert(client.ReadExact(version, sizeof(version)));
    const std::string clientVersion = ProtocolVersion38();
    assert(client.WriteAll(clientVersion.data(), clientVersion.size()));

    CARD8 security[2] = {};
    assert(client.ReadExact(security, sizeof(security)));
    assert(security[0] == 1);
    assert(security[1] == rfbNoAuth);

    CARD8 unsupported = rfbVncAuth;
    assert(client.WriteAll(&unsupported, sizeof(unsupported)));

    server.join();
    assert(!serverResult);
    return 0;
}
