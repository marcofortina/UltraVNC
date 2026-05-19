// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbClientState.h"
#include "vncPortableRfbSession.h"
#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"
#include "vncPortableViewerSession.h"

#include <cassert>
#include <thread>

using namespace uvnc::vncviewer::portable;
using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig serverConfig;
    serverConfig.SetBindAddress("127.0.0.1");
    serverConfig.SetPort(0);
    serverConfig.SetSize(8, 6);
    serverConfig.SetDesktopName("persistent-viewer-test");

    Framebuffer framebuffer;
    assert(framebuffer.Reset(serverConfig.Width(), serverConfig.Height(), serverConfig.PixelFormat()));
    framebuffer.Fill(0x44);

    TcpListener listener;
    assert(listener.Listen(serverConfig.BindAddress(), 0));
    const unsigned short port = listener.Port();

    RfbSessionStats stats;
    RfbClientState state(serverConfig);
    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket client;
        serverOk = listener.Accept(client) &&
                   RfbServerSession().RunHandshake(client, serverConfig) &&
                   RfbServerSession().ServeFramebufferUpdates(client, framebuffer, 2, 16, &stats, &state);
    });

    ViewerConfig viewerConfig;
    viewerConfig.SetHost("127.0.0.1");
    viewerConfig.SetPort(port);
    std::vector<unsigned int> legacyEncodings;
    legacyEncodings.push_back(rfbEncodingRaw);
    legacyEncodings.push_back(rfbEncodingCopyRect);
    legacyEncodings.push_back(rfbEncodingHextile);
    legacyEncodings.push_back(rfbEncodingZlib);
    legacyEncodings.push_back(rfbEncodingZRLE);
    legacyEncodings.push_back(rfbEncodingTight);
    legacyEncodings.push_back(rfbEncodingRRE);
    legacyEncodings.push_back(rfbEncodingCoRRE);
    legacyEncodings.push_back(rfbEncodingNewFBSize);
    viewerConfig.SetEncodings(legacyEncodings);


    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    assert(session.Connect(viewerConfig, result, &error));
    assert(session.Connected());
    assert(result.width == 8);
    assert(result.height == 6);
    assert(result.desktopName == "persistent-viewer-test");

    assert(session.SendKeyEvent(0xff0d, true, &error));
    assert(error.empty());
    assert(session.SendKeyEvent(0xff0d, false, &error));
    assert(session.SendPointerEvent(1, 3, 4, &error));
    assert(session.SendClientCutText("qt-clipboard", &error));

    assert(session.RequestFramebufferUpdate(false, result, &error));
    assert(result.update.received);
    assert(result.update.width == 8);
    assert(result.update.height == 6);

    assert(session.RequestFramebufferUpdate(true, result, &error));
    assert(result.update.received);
    assert(result.update.width == 0);
    assert(result.update.height == 0);
    assert(result.update.pixels.empty());

    session.Disconnect();
    server.join();
    assert(serverOk);
    assert(stats.keyEvents == 2);
    assert(stats.pointerEvents == 1);
    assert(stats.clientCutTextMessages == 1);
    assert(stats.framebufferUpdatesSent == 2);
    assert(state.Encodings().size() >= 2);
    assert(state.Encodings()[0] == rfbEncodingRaw);
    assert(state.Encodings()[1] == rfbEncodingCopyRect);
    assert(state.KeyEventCount() == 2);
    assert(state.LastKeyEvent().keysym == 0xff0d);
    assert(!state.LastKeyEvent().down);
    assert(state.PointerEventCount() == 1);
    assert(state.ClientCutTextMessages() == 1);
    assert(state.ClientCutTextBytes() == 12);
    assert(state.LastPointerEvent().buttonMask == 1);
    assert(state.LastPointerEvent().x == 3);
    assert(state.LastPointerEvent().y == 4);
    return 0;
}
