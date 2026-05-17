// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableTcp.h"

#include <cstring>
#include <iostream>
#include <thread>

using uvnc::winvnc::portable::TcpListener;
using uvnc::winvnc::portable::TcpSocket;

int main()
{
    TcpListener listener;
    if (!listener.Listen("127.0.0.1", 0)) {
        std::cerr << "listen failed\n";
        return 1;
    }
    if (!listener.Valid() || listener.Port() == 0) {
        std::cerr << "listener did not expose an ephemeral port\n";
        return 1;
    }

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        if (!listener.Accept(accepted)) {
            return;
        }
        char request[4] = {};
        if (!accepted.ReadExact(request, sizeof(request))) {
            return;
        }
        if (std::memcmp(request, "ping", sizeof(request)) != 0) {
            return;
        }
        serverOk = accepted.WriteAll("pong", 4);
    });

    TcpSocket client;
    if (!TcpSocket::Connect("127.0.0.1", listener.Port(), client)) {
        std::cerr << "connect failed\n";
        server.join();
        return 1;
    }
    if (!client.WriteAll("ping", 4)) {
        std::cerr << "write failed\n";
        server.join();
        return 1;
    }
    char response[4] = {};
    if (!client.ReadExact(response, sizeof(response))) {
        std::cerr << "read failed\n";
        server.join();
        return 1;
    }
    server.join();
    if (!serverOk || std::memcmp(response, "pong", sizeof(response)) != 0) {
        std::cerr << "unexpected response\n";
        return 1;
    }
    return 0;
}
