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
#include <string>
#include <thread>

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
    if (!client.ReadExact(version, sizeof(version))) {
        return false;
    }
    const std::string clientVersion = ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) {
        return false;
    }
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security)) || security[0] != 1 || security[1] != rfbNoAuth) {
        return false;
    }
    const CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) {
        return false;
    }
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || Swap32IfLE(auth) != rfbVncAuthOK) {
        return false;
    }
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) {
        return false;
    }
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) {
        return false;
    }
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    return client.ReadExact(&name[0], name.size()) &&
           Swap16IfLE(serverInit.framebufferWidth) == config.Width() &&
           Swap16IfLE(serverInit.framebufferHeight) == config.Height();
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetSize(8, 8);
    config.SetFillByte(0x61);

    MemoryServer server;
    assert(server.Start(config));

    bool accepted = false;
    bool serverOk = false;
    std::thread worker([&]() {
        TcpSocket acceptedClient;
        serverOk = server.TryAccept(acceptedClient, 2000, accepted) && accepted &&
                   server.ServeConnectedUpdates(std::move(acceptedClient), 1);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(RunNoAuthHandshake(client, config));

    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 0;
    request.y = 0;
    request.width = config.Width();
    request.height = config.Height();
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(update.nRects) == 1);
    rfbFramebufferUpdateRectHeader header;
    assert(client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap32IfLE(header.encoding) == rfbEncodingRaw);
    std::string pixels(config.Width() * config.Height() * 4, '\0');
    assert(client.ReadExact(&pixels[0], pixels.size()));

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
