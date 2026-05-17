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

using namespace uvnc::winvnc::portable;

namespace {

bool HandshakeClient(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) return false;
    const std::string version38 = ProtocolVersion38();
    if (!client.WriteAll(version38.data(), version38.size())) return false;
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) return false;
    CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) return false;
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != 0) return false;
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    std::string name(Swap32IfLE(serverInit.nameLength), '\0');
    return client.ReadExact(&name[0], name.size()) &&
           Swap16IfLE(serverInit.framebufferWidth) == config.Width() &&
           Swap16IfLE(serverInit.framebufferHeight) == config.Height();
}

bool RequestAndReadUpdate(TcpSocket& client, const ServerConfig& config)
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
    rfbFramebufferUpdateRectHeader rect;
    if (!client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) return false;
    if (Swap16IfLE(rect.r.w) != config.Width() || Swap16IfLE(rect.r.h) != config.Height()) return false;
    std::string pixels(config.Width() * config.Height() * 4, '\0');
    return client.ReadExact(&pixels[0], pixels.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(8, 4);
    config.SetDesktopName("memory-multi-update-smoke");

    MemoryServer server;
    assert(server.Start(config));

    bool serverOk = false;
    std::thread worker([&]() { serverOk = server.ServeOneUpdates(3); });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(HandshakeClient(client, config));
    assert(RequestAndReadUpdate(client, config));
    assert(RequestAndReadUpdate(client, config));
    assert(RequestAndReadUpdate(client, config));

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
