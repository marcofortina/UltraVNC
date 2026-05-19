// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"
#include "vncPortableRfbMessages.h"

#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <sys/socket.h>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string payload = "../escape.txt";
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetFileTransferMode(FileTransferMode::ReadOnly);
    config.SetFileTransferRoot("/tmp/uvnc-ft-root");
    RfbClientState state(config);
    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;
    bool updateSent = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr);
    });

    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbFileTransferRequest;
    message.contentParam = Swap16IfLE(rfbFileTransferVersion);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(clientSocket.WriteAll(&message, sz_rfbFileTransferMsg));
    assert(clientSocket.WriteAll(payload.data(), payload.size()));

    rfbFileTransferMsg abort;
    assert(clientSocket.ReadExact(&abort, sz_rfbFileTransferMsg));
    assert(abort.type == rfbFileTransfer);
    assert(abort.contentType == rfbAbortFileTransfer);
    assert(Swap32IfLE(abort.size) == static_cast<CARD32>(rfbRErrorCmd));

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(stats.fileTransferMessages == 1);
    return 0;
}
