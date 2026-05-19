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
#include <fstream>
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
    if (!payload.empty()) {
        assert(socket.WriteAll(payload.data(), payload.size()));
    }
}

} // namespace

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-recursive-size-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    assert(mkdir((root + "/dir").c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/dir/file.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "abcdef";
    }

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

    SendFileTransferMessage(clientSocket, rfbDirContentRequest, rfbRDirRecursiveSize, "");

    rfbFileTransferMsg response;
    assert(clientSocket.ReadExact(&response, sz_rfbFileTransferMsg));
    assert(response.type == rfbFileTransfer);
    assert(response.contentType == rfbDirPacket);
    assert(Swap16IfLE(response.contentParam) == rfbADirRecursiveSize);
    assert(Swap32IfLE(response.size) == 6);
    std::string payload(Swap32IfLE(response.length), '\0');
    assert(clientSocket.ReadExact(&payload[0], payload.size()));
    assert(payload.find("files=1") != std::string::npos);
    assert(payload.find("directories=1") != std::string::npos);
    assert(payload.find("bytes=6") != std::string::npos);

    worker.join();
    assert(serverOk);
    assert(!updateSent);

    unlink((root + "/dir/file.txt").c_str());
    rmdir((root + "/dir").c_str());
    rmdir(root.c_str());
    return 0;
}
