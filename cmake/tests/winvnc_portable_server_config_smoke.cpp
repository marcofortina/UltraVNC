// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <cassert>
#include <iostream>
#include <string>

using uvnc::winvnc::portable::DefaultFileTransferPayloadLimit;
using uvnc::winvnc::portable::FileTransferMode;
using uvnc::winvnc::portable::ServerConfig;

int main()
{
    ServerConfig config;
    std::string error;
    if (!config.Validate(&error)) {
        std::cerr << "default config rejected: " << error << "\n";
        return 1;
    }
    config.SetSize(0, 480);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid size accepted\n";
        return 1;
    }
    config.SetSize(640, 480);
    rfbPixelFormat format = ServerConfig::DefaultPixelFormat();
    format.bitsPerPixel = 12;
    config.SetPixelFormat(format);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid pixel format accepted\n";
        return 1;
    }
    config.SetPixelFormat(ServerConfig::DefaultPixelFormat());
    config.SetFileTransferMode(FileTransferMode::RejectOnly);
    if (!config.EnableFileTransfer() || config.FileTransferModeValue() != FileTransferMode::RejectOnly) {
        std::cerr << "file transfer reject-only policy not retained\n";
        return 1;
    }
    config.SetFileTransferPayloadLimit(4096);
    if (!config.Validate(&error)) {
        std::cerr << "valid file transfer payload limit rejected: " << error << "\n";
        return 1;
    }
    config.SetFileTransferPayloadLimit(0);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid file transfer payload limit accepted\n";
        return 1;
    }
    config.SetFileTransferPayloadLimit(DefaultFileTransferPayloadLimit());
    config.SetFileTransferRecursiveMaxDepth(8);
    config.SetFileTransferRecursiveMaxEntries(128);
    if (!config.Validate(&error)) {
        std::cerr << "valid recursive file transfer limits rejected: " << error << "\n";
        return 1;
    }
    config.SetFileTransferRecursiveMaxDepth(0);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid recursive depth accepted\n";
        return 1;
    }
    config.SetFileTransferRecursiveMaxDepth(8);
    config.SetFileTransferRecursiveMaxEntries(0);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid recursive entry limit accepted\n";
        return 1;
    }
    config.SetFileTransferRecursiveMaxEntries(128);
    config.SetMaxSharedClients(4);
    if (config.MaxSharedClients() != 4 || !config.Validate(&error)) {
        std::cerr << "valid max shared clients rejected: " << error << "\n";
        return 1;
    }
    config.SetMaxSharedClients(0);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid max shared clients accepted\n";
        return 1;
    }
    config.SetMaxSharedClients(4);
    config.SetExtendedClipboardEnabled(false);
    if (config.ExtendedClipboardEnabled()) {
        std::cerr << "extended clipboard disable flag ignored\n";
        return 1;
    }
    config.SetExtendedClipboardEnabled(true);
    config.SetExtendedClipboardTextLimit(1024);
    if (!config.Validate(&error)) {
        std::cerr << "valid extended clipboard text limit rejected: " << error << "\n";
        return 1;
    }
    config.SetExtendedClipboardTextLimit(0);
    if (config.Validate(&error) || error.empty()) {
        std::cerr << "invalid extended clipboard text limit accepted\n";
        return 1;
    }
    config.SetExtendedClipboardTextLimit(10U * 1024U * 1024U);
    config.SetFileTransferRoot("/tmp");
    assert(config.Validate());
    config.SetFileTransferRoot("relative");
    assert(!config.Validate());

    return 0;
}
