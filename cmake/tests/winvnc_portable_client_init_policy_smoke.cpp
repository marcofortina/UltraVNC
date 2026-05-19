// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"
#include "vncPortableRfb.h"

#include <cassert>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    RfbServerSession session;
    RfbClientState state(config);
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.RunHandshake(serverSocket, config, &state);
    });

    char version[sz_rfbProtocolVersionMsg] = {};
    assert(clientSocket.ReadExact(version, sizeof(version)));
    assert(clientSocket.WriteAll(version, sizeof(version)));
    CARD8 count = 0;
    assert(clientSocket.ReadExact(&count, sizeof(count)));
    CARD8 security = 0;
    assert(clientSocket.ReadExact(&security, sizeof(security)));
    assert(clientSocket.WriteAll(&security, sizeof(security)));
    CARD32 auth = 0;
    assert(clientSocket.ReadExact(&auth, sizeof(auth)));
    assert(auth == AuthOkValue());
    rfbClientInitMsg init;
    init.flags = 0;
    assert(clientSocket.WriteAll(&init, sz_rfbClientInitMsg));
    rfbServerInitMsg serverInit;
    assert(clientSocket.ReadExact(&serverInit, sz_rfbServerInitMsg));
    std::vector<CARD8> name(Swap32IfLE(serverInit.nameLength));
    if (!name.empty()) assert(clientSocket.ReadExact(name.data(), name.size()));

    worker.join();
    assert(serverOk);
    assert(state.ClientInitReceived());
    assert(!state.SharedClientRequested());
    return 0;
}
