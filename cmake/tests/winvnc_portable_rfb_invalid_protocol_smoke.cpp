// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

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
    assert(std::memcmp(version, "RFB 003.008\n", sizeof(version)) == 0);

    const char invalidVersion[sz_rfbProtocolVersionMsg] = "RFB 003.00x";
    assert(client.WriteAll(invalidVersion, sizeof(invalidVersion)));

    server.join();
    assert(!serverResult);
    return 0;
}
