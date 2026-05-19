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
#include <vector>

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
    config.SetSize(4, 4);

    Framebuffer framebuffer(4, 4, config.PixelFormat());
    framebuffer.Fill(0x77);

    TcpListener listener;
    assert(listener.Listen(config.BindAddress(), config.Port()));

    RfbClientState state(config);
    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().RunHandshake(accepted, config) &&
                   RfbServerSession().ServeUntilFramebufferUpdate(accepted, framebuffer, 8, nullptr, &state);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    assert(ClientHandshake(client, config));

    const rfbPixelFormat format16 = ServerConfig::DefaultPixelFormat();
    rfbPixelFormat wireFormat = format16;
    wireFormat.bitsPerPixel = 16;
    wireFormat.depth = 16;
    wireFormat.redMax = 31;
    wireFormat.greenMax = 63;
    wireFormat.blueMax = 31;
    const rfbSetPixelFormatMsg setFormat = EncodeSetPixelFormat(wireFormat);
    assert(client.WriteAll(&setFormat, sz_rfbSetPixelFormatMsg));

    const std::vector<CARD8> encodings = EncodeSetEncodings(std::vector<CARD32>{rfbEncodingRaw, rfbEncodingTight});
    assert(client.WriteAll(encodings.data(), encodings.size()));

    const KeyEvent key{false, 0xff1b};
    const rfbKeyEventMsg keyWire = EncodeKeyEvent(key);
    assert(client.WriteAll(&keyWire, sz_rfbKeyEventMsg));

    const PointerEvent pointer{2, 3, 1};
    const rfbPointerEventMsg pointerWire = EncodePointerEvent(pointer);
    assert(client.WriteAll(&pointerWire, sz_rfbPointerEventMsg));

    rfbClientCutTextMsg cut;
    std::memset(&cut, 0, sizeof(cut));
    cut.type = rfbClientCutText;
    cut.length = Swap32IfLE(3);
    assert(client.WriteAll(&cut, sz_rfbClientCutTextMsg));
    assert(client.WriteAll("abc", 3));

    const FramebufferUpdateRequest request{false, 0, 0, 4, 4};
    const rfbFramebufferUpdateRequestMsg updateRequest = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&updateRequest, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    rfbFramebufferUpdateRectHeader rect;
    assert(client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap32IfLE(rect.encoding) == rfbEncodingRaw);
    const unsigned int bytesPerPixel = wireFormat.bitsPerPixel / 8;
    std::string pixels(4 * 4 * bytesPerPixel, '\0');
    assert(client.ReadExact(&pixels[0], pixels.size()));

    server.join();
    listener.Close();
    assert(serverOk);
    assert(state.PixelFormat().bitsPerPixel == 16);
    assert(state.Encodings().size() == 2);
    assert(state.Encodings()[0] == rfbEncodingRaw);
    assert(state.Encodings()[1] == rfbEncodingTight);
    assert(state.KeyEventCount() == 1);
    assert(state.LastKeyEvent().keysym == 0xff1b);
    assert(state.PointerEventCount() == 1);
    assert(state.LastPointerEvent().buttonMask == 2);
    assert(state.ClientCutTextMessages() == 1);
    assert(state.ClientCutTextBytes() == 3);
    return 0;
}
