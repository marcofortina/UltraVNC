// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerFileTransfer.h"
#include "vncPortableRfbSession.h"

#include <cassert>
#include <fstream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::vncviewer::portable;
using namespace uvnc::winvnc::portable;

namespace {

class ServerOnce {
public:
    ServerOnce(TcpSocket& serverSocket, RfbClientState& state)
        : serverSocket_(serverSocket), state_(state), ok_(false)
    {
        worker_ = std::thread([this]() {
            RfbServerSession session;
            Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
            bool updateSent = false;
            ok_ = session.ServeNextClientMessage(serverSocket_, framebuffer, updateSent, nullptr, &state_);
        });
    }

    ~ServerOnce()
    {
        Wait();
    }

    bool Wait()
    {
        if (worker_.joinable()) worker_.join();
        return ok_;
    }

    bool ok() const { return ok_; }

private:
    TcpSocket& serverSocket_;
    RfbClientState& state_;
    bool ok_;
    std::thread worker_;
};

} // namespace

int main()
{
    const std::string root = std::string("/tmp/uvnc-viewer-ft-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    assert(mkdir((root + "/dir").c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/dir/file.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "viewer-ft-payload";
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

    {
        ServerOnce server(serverSocket, state);
        std::vector<ViewerFileTransferEntry> entries;
        std::string error;
        assert(RequestViewerDirectoryListing(clientSocket, "dir", entries, &error));
        assert(server.Wait());
        assert(!entries.empty());
        bool sawFile = false;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (!entries[i].directory && entries[i].name == "file.txt") {
                sawFile = true;
            }
        }
        assert(sawFile);
    }

    {
        ServerOnce server(serverSocket, state);
        ViewerFileDownload download;
        std::string error;
        assert(RequestViewerFileDownload(clientSocket, "dir/file.txt", download, &error));
        assert(server.Wait());
        assert(download.expectedSize == std::string("viewer-ft-payload").size());
        assert(std::string(download.payload.begin(), download.payload.end()) == "viewer-ft-payload");
    }

    {
        ServerOnce server(serverSocket, state);
        std::vector<std::string> checksums;
        std::string error;
        assert(RequestViewerFileChecksums(clientSocket, "dir/file.txt", checksums, &error));
        assert(server.Wait());
        assert(!checksums.empty());
    }

    unlink((root + "/dir/file.txt").c_str());
    rmdir((root + "/dir").c_str());
    rmdir(root.c_str());
    return 0;
}
