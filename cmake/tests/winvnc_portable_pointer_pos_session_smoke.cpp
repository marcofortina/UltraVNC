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

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(8, 8);

    Framebuffer framebuffer(8, 8, config.PixelFormat());
    framebuffer.Fill(0x33);

    TcpListener listener;
    assert(listener.Listen(config.BindAddress(), config.Port()));

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        RfbClientState state(config);
        RfbSessionStats stats;
        bool updateSent = false;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().RunHandshake(accepted, config) &&
                   RfbServerSession().ServeNextClientMessage(accepted, framebuffer, updateSent, &stats, &state) &&
                   RfbServerSession().ServeNextClientMessage(accepted, framebuffer, updateSent, &stats, &state);
        assert(stats.setEncodingsMessages == 1);
        assert(stats.pointerEvents == 1);
        assert(stats.pointerPositionUpdatesSent == 1);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    assert(ClientHandshake(client, config));

    const std::vector<CARD8> encodings = EncodeSetEncodings(std::vector<CARD32>{rfbEncodingPointerPos, rfbEncodingRaw});
    assert(client.WriteAll(encodings.data(), encodings.size()));

    const PointerEvent pointer{0, 5, 6};
    const rfbPointerEventMsg pointerWire = EncodePointerEvent(pointer);
    assert(client.WriteAll(&pointerWire, sz_rfbPointerEventMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(update.nRects) == 1);

    rfbFramebufferUpdateRectHeader rect;
    assert(client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap16IfLE(rect.r.x) == 5);
    assert(Swap16IfLE(rect.r.y) == 6);
    assert(Swap16IfLE(rect.r.w) == 0);
    assert(Swap16IfLE(rect.r.h) == 0);
    assert(Swap32IfLE(rect.encoding) == rfbEncodingPointerPos);

    server.join();
    listener.Close();
    assert(serverOk);
    return 0;
}
