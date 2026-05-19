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
#include <set>
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
    const std::string root = std::string("/tmp/uvnc-ft-dir-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    assert(mkdir((root + "/subdir").c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/hello.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "hello";
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
    bool serverOk = false;
    bool updateSent = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr);
    });

    SendFileTransferMessage(clientSocket, rfbDirContentRequest, rfbRDirContent, "");

    std::set<std::string> directories;
    std::set<std::string> files;
    bool sawTerminator = false;
    for (unsigned int i = 0; i < 8 && !sawTerminator; ++i) {
        rfbFileTransferMsg response;
        assert(clientSocket.ReadExact(&response, sz_rfbFileTransferMsg));
        assert(response.type == rfbFileTransfer);
        assert(response.contentType == rfbDirPacket);
        const CARD16 param = Swap16IfLE(response.contentParam);
        const CARD32 length = Swap32IfLE(response.length);
        const std::string payload = ReadPayload(clientSocket, length);
        if (param == 0 && length == 0) {
            sawTerminator = true;
        } else if (param == rfbADirectory && !payload.empty()) {
            directories.insert(payload);
        } else if (param == rfbAFile) {
            files.insert(payload);
        }
    }

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(stats.fileTransferMessages == 1);
    assert(sawTerminator);
    assert(directories.count("subdir") == 1);
    assert(files.count("hello.txt") == 1);

    unlink((root + "/hello.txt").c_str());
    rmdir((root + "/subdir").c_str());
    rmdir(root.c_str());
    return 0;
}
