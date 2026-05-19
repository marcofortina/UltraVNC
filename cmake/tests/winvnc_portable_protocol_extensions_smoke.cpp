// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbClientState.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbSession.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

void WriteScale(TcpSocket& socket, CARD8 type, CARD8 scale)
{
    rfbSetScaleMsg msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.type = type;
    msg.scale = scale;
    assert(socket.WriteAll(&msg, sz_rfbSetScaleMsg));
}

} // namespace

int main()
{
    RfbClientState state;
    state.SetEncodings(std::vector<CARD32>{rfbEncodingRaw, rfbEncodingLastRect, rfbEncodingQualityLevel7, rfbEncodingCompressLevel3});
    assert(state.SupportsLastRect());
    assert(state.QualityLevel() == 7);
    assert(state.CompressLevel() == 3);

    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    bool updateSent = false;
    RfbSessionStats stats;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state);
    });
    WriteScale(clientSocket, rfbSetScale, 4);
    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(state.ScaleFactor() == 4);
    assert(stats.setScaleMessages == 1);
    return 0;
}
