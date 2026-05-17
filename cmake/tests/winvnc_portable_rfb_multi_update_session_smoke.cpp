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

bool ReadUpdate(TcpSocket& client, unsigned int width, unsigned int height)
{
    rfbFramebufferUpdateMsg update;
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) return false;
    rfbFramebufferUpdateRectHeader rect;
    if (!client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) return false;
    if (Swap16IfLE(rect.r.w) != width || Swap16IfLE(rect.r.h) != height || Swap32IfLE(rect.encoding) != rfbEncodingRaw) return false;
    std::string pixels(width * height * 4, '\0');
    return client.ReadExact(&pixels[0], pixels.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(4, 3);

    Framebuffer framebuffer(4, 3, config.PixelFormat());
    framebuffer.Fill(0x55);

    TcpListener listener;
    assert(listener.Listen(config.BindAddress(), config.Port()));

    RfbSessionStats stats;
    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().RunHandshake(accepted, config) &&
                   RfbServerSession().ServeFramebufferUpdates(accepted, framebuffer, 2, 4, &stats);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    assert(ClientHandshake(client, config));

    FramebufferUpdateRequest request{false, 0, 0, 4, 3};
    rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadUpdate(client, 4, 3));
    request.incremental = true;
    wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadUpdate(client, 4, 3));

    server.join();
    listener.Close();
    assert(serverOk);
    assert(stats.framebufferUpdatesSent == 2);
    assert(stats.messagesProcessed == 2);
    return 0;
}
