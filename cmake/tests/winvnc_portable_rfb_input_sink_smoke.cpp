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
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

class RecordingInputSink : public RfbInputSink {
public:
    RecordingInputSink() : keyEvents(0), pointerEvents(0), lastKeysym(0), lastButtonMask(0) {}

    bool InjectKey(const KeyEvent& event, std::string *error) override
    {
        (void)error;
        keyEvents += 1;
        lastKeysym = event.keysym;
        return true;
    }

    bool InjectPointer(const PointerEvent& event, std::string *error) override
    {
        (void)error;
        pointerEvents += 1;
        lastButtonMask = event.buttonMask;
        return true;
    }

    unsigned int keyEvents;
    unsigned int pointerEvents;
    CARD32 lastKeysym;
    CARD8 lastButtonMask;
};

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
    framebuffer.Fill(0x44);

    TcpListener listener;
    assert(listener.Listen(config.BindAddress(), config.Port()));

    RecordingInputSink sink;
    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().RunHandshake(accepted, config) &&
                   RfbServerSession().ServeUntilFramebufferUpdate(accepted, framebuffer, 32, nullptr, nullptr, &sink);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    assert(ClientHandshake(client, config));

    const KeyEvent key{true, 0xff0d};
    const rfbKeyEventMsg keyWire = EncodeKeyEvent(key);
    assert(client.WriteAll(&keyWire, sz_rfbKeyEventMsg));

    const PointerEvent pointer{1, 2, 3};
    const rfbPointerEventMsg pointerWire = EncodePointerEvent(pointer);
    assert(client.WriteAll(&pointerWire, sz_rfbPointerEventMsg));

    FramebufferUpdateRequest request{false, 0, 0, 4, 4};
    const rfbFramebufferUpdateRequestMsg updateRequest = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&updateRequest, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(update.nRects) == 1);

    rfbFramebufferUpdateRectHeader rect;
    assert(client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader));
    std::string pixels(4 * 4 * 4, '\0');
    assert(client.ReadExact(&pixels[0], pixels.size()));

    server.join();
    listener.Close();
    assert(serverOk);
    assert(sink.keyEvents == 1);
    assert(sink.pointerEvents == 1);
    assert(sink.lastKeysym == 0xff0d);
    assert(sink.lastButtonMask == 1);
    return 0;
}
