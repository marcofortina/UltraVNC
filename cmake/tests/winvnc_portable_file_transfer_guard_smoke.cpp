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
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string payload = "ignored-file-transfer-payload";
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;
    bool updateSent = false;
    bool serverOk = false;
    RfbClientState state;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr);
    });

    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbFileTransferOffer;
    message.contentParam = Swap16IfLE(rfbFileTransferVersion);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(clientSocket.WriteAll(&message, sz_rfbFileTransferMsg));
    assert(clientSocket.WriteAll(payload.data(), payload.size()));

    rfbFileTransferMsg abort;
    assert(clientSocket.ReadExact(&abort, sz_rfbFileTransferMsg));
    assert(abort.type == rfbFileTransfer);
    assert(abort.contentType == rfbAbortFileTransfer);
    assert(Swap16IfLE(abort.contentParam) == rfbFileTransferVersion);

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(stats.fileTransferMessages == 1);
    assert(stats.fileTransferBytesDiscarded == payload.size());
    return 0;
}
