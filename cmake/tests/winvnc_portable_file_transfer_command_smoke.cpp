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

void SendCommand(TcpSocket& socket, CARD16 command, const std::string& payload)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbCommand;
    message.contentParam = Swap16IfLE(command);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(socket.WriteAll(&message, sz_rfbFileTransferMsg));
    if (!payload.empty()) {
        assert(socket.WriteAll(payload.data(), payload.size()));
    }
}

std::string ReadCommandReturn(TcpSocket& socket, CARD16 expectedParam, CARD32 expectedStatus)
{
    rfbFileTransferMsg response;
    assert(socket.ReadExact(&response, sz_rfbFileTransferMsg));
    assert(response.type == rfbFileTransfer);
    assert(response.contentType == rfbCommandReturn);
    assert(Swap16IfLE(response.contentParam) == expectedParam);
    assert(Swap32IfLE(response.size) == expectedStatus);
    std::string payload(Swap32IfLE(response.length), '\0');
    if (!payload.empty()) {
        assert(socket.ReadExact(&payload[0], payload.size()));
    }
    return payload;
}

void RunCommand(RfbServerSession& session,
                TcpSocket& serverSocket,
                TcpSocket& clientSocket,
                const Framebuffer& framebuffer,
                RfbSessionStats& stats,
                RfbClientState& state,
                CARD16 command,
                CARD16 expectedParam,
                const std::string& payload)
{
    bool serverOk = false;
    std::thread worker([&]() { serverOk = ServeOne(session, serverSocket, framebuffer, stats, state); });
    SendCommand(clientSocket, command, payload);
    assert(ReadCommandReturn(clientSocket, expectedParam, 0) == payload);
    worker.join();
    assert(serverOk);
}

} // namespace

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-command-") + std::to_string(getpid());
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

    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCDirCreate, rfbADirCreate, "dir1");
    assert(mkdir((root + "/dir1").c_str(), 0700) != 0);
    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCFileCreate, rfbAFileCreate, "file1.txt");
    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCFileRename, rfbAFileRename, "file1.txt*file2.txt");
    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCFileDelete, rfbAFileDelete, "file2.txt");
    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCDirRename, rfbADirRename, "dir1*dir2");
    RunCommand(session, serverSocket, clientSocket, framebuffer, stats, state, rfbCDirDelete, rfbADirDelete, "dir2");

    rmdir(root.c_str());
    return 0;
}
