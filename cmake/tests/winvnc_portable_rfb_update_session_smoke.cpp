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
#include <cstring>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

bool ClientHandshake(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) {
        return false;
    }
    const std::string version38 = ProtocolVersion38();
    if (!client.WriteAll(version38.data(), version38.size())) {
        return false;
    }
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) {
        return false;
    }
    CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) {
        return false;
    }
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != 0) {
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
           Swap16IfLE(serverInit.framebufferHeight) == config.Height() &&
           name == config.DesktopName();
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(4, 3);
    config.SetDesktopName("update-session");

    Framebuffer framebuffer(4, 3, config.PixelFormat());
    framebuffer.Fill(0x33);

    TcpListener listener;
    assert(listener.Listen(config.BindAddress(), config.Port()));

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().RunHandshake(accepted, config) &&
                   RfbServerSession().ServeFramebufferUpdateRequest(accepted, framebuffer);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    assert(ClientHandshake(client, config));

    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 0;
    request.y = 0;
    request.width = 4;
    request.height = 3;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(update.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(update.nRects) == 1);

    rfbFramebufferUpdateRectHeader header;
    assert(client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap16IfLE(header.r.w) == 4);
    assert(Swap16IfLE(header.r.h) == 3);
    assert(Swap32IfLE(header.encoding) == rfbEncodingRaw);

    std::string pixels(4 * 3 * 4, '\0');
    assert(client.ReadExact(&pixels[0], pixels.size()));

    server.join();
    listener.Close();
    assert(serverOk);
    return 0;
}
