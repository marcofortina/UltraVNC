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

namespace {

bool ServeOne(RfbServerSession& session, TcpSocket& socket, const Framebuffer& framebuffer, RfbSessionStats& stats, RfbClientState& state)
{
    bool updateSent = false;
    return session.ServeNextClientMessage(socket, framebuffer, updateSent, &stats, &state, nullptr) && !updateSent;
}

void SendAccessRequest(TcpSocket& socket)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbFileTransferAccess;
    message.contentParam = Swap16IfLE(rfbFileTransferVersion);
    assert(socket.WriteAll(&message, sz_rfbFileTransferMsg));
}

CARD32 ReadAccessStatus(TcpSocket& socket)
{
    rfbFileTransferMsg response;
    assert(socket.ReadExact(&response, sz_rfbFileTransferMsg));
    assert(response.type == rfbFileTransfer);
    assert(response.contentType == rfbFileTransferAccess);
    assert(Swap16IfLE(response.contentParam) == rfbFileTransferVersion);
    return Swap32IfLE(response.size);
}

void ExpectAccess(FileTransferMode mode, CARD32 expected)
{
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetFileTransferMode(mode);
    if (mode == FileTransferMode::ReadOnly || mode == FileTransferMode::ReadWrite) {
        config.SetFileTransferRoot("/tmp");
    }
    RfbClientState state(config);
    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;
    bool serverOk = false;
    std::thread worker([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });

    SendAccessRequest(clientSocket);
    assert(ReadAccessStatus(clientSocket) == expected);
    worker.join();
    assert(serverOk);
    assert(stats.fileTransferMessages == 1);
}

} // namespace

int main()
{
    ExpectAccess(FileTransferMode::Disabled, static_cast<CARD32>(rfbRErrorCmd));
    ExpectAccess(FileTransferMode::RejectOnly, static_cast<CARD32>(rfbRErrorCmd));
    ExpectAccess(FileTransferMode::ReadOnly, 1);
    ExpectAccess(FileTransferMode::ReadWrite, 1);
    return 0;
}
