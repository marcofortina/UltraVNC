// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableClientPolicy.h"
#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <string>
#include <thread>
#include <vector>

using uvnc::winvnc::portable::ClientConnectionPolicy;
using uvnc::winvnc::portable::EncodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::FramebufferUpdateRequest;
using uvnc::winvnc::portable::MemoryServer;
using uvnc::winvnc::portable::ProtocolVersion38;
using uvnc::winvnc::portable::ServerConfig;
using uvnc::winvnc::portable::TcpSocket;

namespace {

bool RunNoAuthHandshake(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) return false;
    const std::string clientVersion = ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) return false;
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security)) || security[0] != 1 || security[1] != rfbNoAuth) return false;
    const CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) return false;
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || Swap32IfLE(auth) != rfbVncAuthOK) return false;
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    return client.ReadExact(&name[0], name.size()) &&
           Swap16IfLE(serverInit.framebufferWidth) == config.Width() &&
           Swap16IfLE(serverInit.framebufferHeight) == config.Height();
}

bool RequestOneRawUpdate(TcpSocket& client, const ServerConfig& config)
{
    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 0;
    request.y = 0;
    request.width = config.Width();
    request.height = config.Height();
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    if (!client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg)) return false;
    rfbFramebufferUpdateMsg update;
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) return false;
    rfbFramebufferUpdateRectHeader header;
    if (!client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader) || Swap32IfLE(header.encoding) != rfbEncodingRaw) return false;
    std::string pixels(config.Width() * config.Height() * 4, '\0');
    return client.ReadExact(&pixels[0], pixels.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetSize(8, 8);
    config.SetMaxSharedClients(2);

    MemoryServer server;
    assert(server.Start(config));
    ClientConnectionPolicy policy(config.MaxSharedClients());

    std::vector<std::thread> workers;
    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([&]() {
            TcpSocket acceptedClient;
            bool accepted = false;
            const bool ok = server.TryAccept(acceptedClient, 2000, accepted) && accepted &&
                            server.ServeConnectedUpdates(std::move(acceptedClient), 1, nullptr, nullptr, nullptr, &policy);
            assert(ok);
        });
    }

    std::vector<std::thread> clients;
    for (int i = 0; i < 2; ++i) {
        clients.emplace_back([&]() {
            TcpSocket client;
            assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
            assert(RunNoAuthHandshake(client, config));
            assert(RequestOneRawUpdate(client, config));
        });
    }

    for (std::size_t i = 0; i < clients.size(); ++i) clients[i].join();
    for (std::size_t i = 0; i < workers.size(); ++i) workers[i].join();
    server.Stop();
    assert(policy.ActiveClients() == 0);
    return 0;
}
