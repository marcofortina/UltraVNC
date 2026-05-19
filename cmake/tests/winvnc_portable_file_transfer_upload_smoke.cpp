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

bool ServeOne(RfbServerSession& session, TcpSocket& socket, const Framebuffer& framebuffer, RfbSessionStats& stats, RfbClientState& state)
{
    bool updateSent = false;
    return session.ServeNextClientMessage(socket, framebuffer, updateSent, &stats, &state, nullptr) && !updateSent;
}

void SendFileTransferMessage(TcpSocket& socket, CARD8 contentType, CARD16 contentParam, CARD32 size, const std::string& payload)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = contentType;
    message.contentParam = Swap16IfLE(contentParam);
    message.size = Swap32IfLE(size);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(socket.WriteAll(&message, sz_rfbFileTransferMsg));
    if (!payload.empty()) {
        assert(socket.WriteAll(payload.data(), payload.size()));
    }
}

std::string ReadFile(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-upload-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);

    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetFileTransferMode(FileTransferMode::ReadWrite);
    config.SetFileTransferRoot(root);
    RfbClientState state(config);
    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;

    bool serverOk = false;
    std::thread offerWorker([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });
    SendFileTransferMessage(clientSocket, rfbFileTransferOffer, rfbFileTransferVersion, 0, "upload.txt");
    rfbFileTransferMsg accept;
    assert(clientSocket.ReadExact(&accept, sz_rfbFileTransferMsg));
    assert(accept.type == rfbFileTransfer);
    assert(accept.contentType == rfbFileAcceptHeader);
    assert(Swap32IfLE(accept.size) == 1);
    offerWorker.join();
    assert(serverOk);
    assert(state.FileUploadActive());
    struct stat uploadStat;
    assert(stat((root + "/upload.txt").c_str(), &uploadStat) != 0);
    assert(stat((root + "/upload.txt.uvnc-upload.tmp").c_str(), &uploadStat) == 0);

    std::thread packetWorker([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });
    SendFileTransferMessage(clientSocket, rfbFilePacket, 0, 0, "hello-");
    packetWorker.join();
    assert(serverOk);

    std::thread packetWorker2([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });
    SendFileTransferMessage(clientSocket, rfbFilePacket, 0, 6, "upload");
    packetWorker2.join();
    assert(serverOk);

    std::thread eofWorker([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });
    SendFileTransferMessage(clientSocket, rfbEndOfFile, 0, 12, "");
    eofWorker.join();
    assert(serverOk);
    assert(!state.FileUploadActive());
    assert(ReadFile(root + "/upload.txt") == "hello-upload");
    assert(stat((root + "/upload.txt.uvnc-upload.tmp").c_str(), &uploadStat) != 0);

    unlink((root + "/upload.txt").c_str());
    rmdir(root.c_str());
    return 0;
}
