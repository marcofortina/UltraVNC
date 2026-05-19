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
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

namespace {

void SendFileTransferMessage(TcpSocket& socket, CARD8 contentType, CARD16 contentParam, const std::string& payload)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = contentType;
    message.contentParam = Swap16IfLE(contentParam);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(socket.WriteAll(&message, sz_rfbFileTransferMsg));
}

std::string ReadPayload(TcpSocket& socket, CARD32 length)
{
    std::string payload(length, '\0');
    if (!payload.empty()) {
        assert(socket.ReadExact(&payload[0], payload.size()));
    }
    return payload;
}

} // namespace

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-drives-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);

    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetFileTransferMode(FileTransferMode::ReadOnly);
    config.SetFileTransferRoot(root);
    RfbClientState state(config);
    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;
    bool updateSent = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr);
    });

    SendFileTransferMessage(clientSocket, rfbDirContentRequest, rfbRDrivesList, "");

    rfbFileTransferMsg response;
    assert(clientSocket.ReadExact(&response, sz_rfbFileTransferMsg));
    assert(response.type == rfbFileTransfer);
    assert(response.contentType == rfbDirPacket);
    assert(Swap16IfLE(response.contentParam) == rfbADrivesList);
    const std::string payload = ReadPayload(clientSocket, Swap32IfLE(response.length));
    assert(payload == root);
    assert(clientSocket.ReadExact(&response, sz_rfbFileTransferMsg));
    assert(response.contentType == rfbDirPacket);
    assert(Swap16IfLE(response.contentParam) == 0);
    assert(Swap32IfLE(response.length) == 0);

    worker.join();
    assert(serverOk);
    assert(!updateSent);

    rmdir(root.c_str());
    return 0;
}
