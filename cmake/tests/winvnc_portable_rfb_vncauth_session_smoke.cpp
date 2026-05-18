// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include "d3des.h"
}

using namespace uvnc::winvnc::portable;

namespace {

void EncryptChallenge(std::vector<CARD8>& challenge, const std::string& password)
{
    unsigned char key[8] = {};
    for (std::size_t i = 0; i < sizeof(key) && i < password.size(); ++i) {
        key[i] = static_cast<unsigned char>(password[i]);
    }
    deskey(key, EN0);
    for (std::size_t i = 0; i + 8 <= challenge.size(); i += 8) {
        des(challenge.data() + i, challenge.data() + i);
    }
}

bool RunPasswordHandshake(TcpSocket& client, const std::string& password, bool expectSuccess)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) {
        return false;
    }
    const std::string clientVersion = ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) {
        return false;
    }

    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security)) || security[0] != 1 || security[1] != rfbVncAuth) {
        return false;
    }

    const CARD8 selected = rfbVncAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) {
        return false;
    }

    std::vector<CARD8> challenge(16);
    if (!client.ReadExact(challenge.data(), challenge.size())) {
        return false;
    }
    EncryptChallenge(challenge, password);
    if (!client.WriteAll(challenge.data(), challenge.size())) {
        return false;
    }

    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth))) {
        return false;
    }
    if (Swap32IfLE(auth) != (expectSuccess ? rfbVncAuthOK : rfbVncAuthFailed)) {
        return false;
    }
    if (!expectSuccess) {
        return true;
    }

    rfbClientInitMsg init;
    init.flags = 1;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) {
        return false;
    }

    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) {
        return false;
    }
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::vector<CARD8> name(nameLength);
    return name.empty() || client.ReadExact(name.data(), name.size());
}

bool RunOneSession(const std::string& clientPassword, bool expectSuccess)
{
    ServerConfig config;
    config.SetSize(64, 32);
    config.SetDesktopName("vncauth-server-smoke");
    config.SetAuthMode(ServerAuthMode::VncPassword);
    config.SetVncPassword("secret1");

    MemoryServer server;
    if (!server.Start(config)) {
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOne();
    });

    TcpSocket client;
    const bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                          RunPasswordHandshake(client, clientPassword, expectSuccess);
    worker.join();
    server.Stop();
    return clientOk && serverOk == expectSuccess;
}

} // namespace

int main()
{
    assert(RunOneSession("secret1", true));
    assert(RunOneSession("wrong", false));
    return 0;
}
