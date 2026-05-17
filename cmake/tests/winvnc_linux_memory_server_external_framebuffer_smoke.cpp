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
#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

bool Handshake(TcpSocket& client, const ServerConfig& config)
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

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(3, 2);
    config.SetDesktopName("external-framebuffer-smoke");

    Framebuffer framebuffer(3, 2, config.PixelFormat());
    framebuffer.Fill(0x7b);

    MemoryServer server;
    assert(server.StartWithFramebuffer(config, framebuffer));

    bool serverOk = false;
    std::thread worker([&]() { serverOk = server.ServeOneUpdate(); });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(Handshake(client, config));

    FramebufferUpdateRequest request{false, 0, 0, 3, 2};
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(update.nRects) == 1);
    rfbFramebufferUpdateRectHeader rect;
    assert(client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader));
    std::string pixels(3 * 2 * 4, '\0');
    assert(client.ReadExact(&pixels[0], pixels.size()));
    assert(static_cast<unsigned char>(pixels[0]) == 0x7b);

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
