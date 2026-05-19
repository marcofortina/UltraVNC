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

void SendFileTransferRequest(TcpSocket& socket, CARD8 contentType, CARD16 contentParam, const std::string& payload)
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
    const std::string root = std::string("/tmp/uvnc-ft-download-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/hello.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "hello-download";
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

    SendFileTransferRequest(clientSocket, rfbFileTransferRequest, rfbFileTransferVersion, "hello.txt");

    rfbFileTransferMsg header;
    assert(clientSocket.ReadExact(&header, sz_rfbFileTransferMsg));
    assert(header.type == rfbFileTransfer);
    assert(header.contentType == rfbFileHeader);
    assert(Swap32IfLE(header.size) == 14);
    assert(ReadPayload(clientSocket, Swap32IfLE(header.length)) == "hello.txt");

    rfbFileTransferMsg packet;
    assert(clientSocket.ReadExact(&packet, sz_rfbFileTransferMsg));
    assert(packet.type == rfbFileTransfer);
    assert(packet.contentType == rfbFilePacket);
    assert(ReadPayload(clientSocket, Swap32IfLE(packet.length)) == "hello-download");

    rfbFileTransferMsg eof;
    assert(clientSocket.ReadExact(&eof, sz_rfbFileTransferMsg));
    assert(eof.type == rfbFileTransfer);
    assert(eof.contentType == rfbEndOfFile);
    assert(Swap32IfLE(eof.size) == 14);

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(stats.fileTransferMessages == 1);
    unlink((root + "/hello.txt").c_str());
    rmdir(root.c_str());
    return 0;
}
