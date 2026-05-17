// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebuffer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbSession.h"
#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <string>
#include <sys/socket.h>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

bool ClientHandshake(TcpSocket& client, const ServerConfig& config)
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
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    ServerConfig config;
    config.SetSize(4, 4);
    Framebuffer framebuffer(4, 4, config.PixelFormat());
    framebuffer.Fill(0x12);

    bool serverResult = true;
    RfbSessionStats stats;
    RfbClientState state(config);
    std::thread server([&]() {
        TcpSocket socket(fds[0]);
        serverResult = RfbServerSession().RunHandshake(socket, config) &&
                       RfbServerSession().ServeUntilFramebufferUpdate(socket, framebuffer, 1, &stats, &state);
    });

    TcpSocket client(fds[1]);
    assert(ClientHandshake(client, config));
    const KeyEvent key{true, 0xff0d};
    const rfbKeyEventMsg keyWire = EncodeKeyEvent(key);
    assert(client.WriteAll(&keyWire, sz_rfbKeyEventMsg));

    server.join();
    assert(!serverResult);
    assert(stats.messagesProcessed == 1);
    assert(stats.keyEvents == 1);
    assert(state.KeyEventCount() == 1);
    return 0;
}
