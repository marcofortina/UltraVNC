// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExtendedClipboard.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbSession.h"

#include <cassert>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    config.SetExtendedClipboardEnabled(false);
    RfbClientState state(config);
    assert(!state.SupportsExtendedClipboard());
    state.SetEncodings(std::vector<CARD32>(1, rfbEncodingExtendedClipboard));
    assert(!state.SupportsExtendedClipboard());

    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    bool updateSent = false;
    bool serverOk = true;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, nullptr, &state);
    });

    const std::vector<CARD8> provide = EncodeExtendedClientCutText(EncodeExtendedClipboardProvideText("blocked"));
    assert(clientSocket.WriteAll(provide.data(), provide.size()));
    worker.join();
    assert(!serverOk);
    return 0;
}
