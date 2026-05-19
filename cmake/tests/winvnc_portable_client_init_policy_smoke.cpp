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

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    auto pair = TcpSocket::CreateConnectedPair();
    assert(pair.first.Valid());
    assert(pair.second.Valid());

    RfbServerSession session;
    RfbClientState state(config);
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.RunHandshake(pair.first, config, &state);
    });

    char version[sz_rfbProtocolVersionMsg] = {};
    assert(pair.second.ReadExact(version, sizeof(version)));
    assert(pair.second.WriteAll(version, sizeof(version)));
    CARD8 count = 0;
    assert(pair.second.ReadExact(&count, sizeof(count)));
    CARD8 security = 0;
    assert(pair.second.ReadExact(&security, sizeof(security)));
    assert(pair.second.WriteAll(&security, sizeof(security)));
    CARD32 auth = 0;
    assert(pair.second.ReadExact(&auth, sizeof(auth)));
    assert(auth == AuthOkValue());
    rfbClientInitMsg init;
    init.shared = 0;
    assert(pair.second.WriteAll(&init, sz_rfbClientInitMsg));
    rfbServerInitMsg serverInit;
    assert(pair.second.ReadExact(&serverInit, sz_rfbServerInitMsg));
    std::vector<CARD8> name(Swap32IfLE(serverInit.nameLength));
    if (!name.empty()) assert(pair.second.ReadExact(name.data(), name.size()));

    worker.join();
    assert(serverOk);
    assert(state.ClientInitReceived());
    assert(!state.SharedClientRequested());
    return 0;
}
