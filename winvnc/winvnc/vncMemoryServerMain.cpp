// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"

#include <cstdlib>
#include <iostream>
#include <string>

using uvnc::winvnc::portable::ServerConfig;
using uvnc::winvnc::portable::MemoryServer;

namespace {

void PrintUsage(const char *name)
{
    std::cout << "Usage: " << name << " [options]\n"
              << "\n"
              << "Experimental native Linux WinVNC memory server.\n"
              << "This target serves an in-memory framebuffer skeleton and does not capture a real desktop yet.\n"
              << "\n"
              << "Options:\n"
              << "  --bind-address <ipv4>   Bind address, default 127.0.0.1\n"
              << "  --port <port>           RFB port, default 0 for ephemeral\n"
              << "  --width <pixels>        Framebuffer width, default 640\n"
              << "  --height <pixels>       Framebuffer height, default 480\n"
              << "  --name <text>           Desktop name\n"
              << "  --validate-config       Validate options and exit\n"
              << "  --help                  Show this help\n";
}

bool ParseUnsigned(const char *value, unsigned int min, unsigned int max, unsigned int& out)
{
    if (!value || !*value) {
        return false;
    }
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (*end != '\0' || parsed < min || parsed > max) {
        return false;
    }
    out = static_cast<unsigned int>(parsed);
    return true;
}

bool ParseArgs(int argc, char **argv, ServerConfig& config, bool& validateOnly)
{
    validateOnly = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (arg == "--validate-config") {
            validateOnly = true;
        } else if (arg == "--bind-address" && i + 1 < argc) {
            config.SetBindAddress(argv[++i]);
        } else if (arg == "--name" && i + 1 < argc) {
            config.SetDesktopName(argv[++i]);
        } else if (arg == "--port" && i + 1 < argc) {
            unsigned int port = 0;
            if (!ParseUnsigned(argv[++i], 0, 65535, port)) {
                std::cerr << "invalid --port\n";
                return false;
            }
            config.SetPort(static_cast<unsigned short>(port));
        } else if (arg == "--width" && i + 1 < argc) {
            unsigned int width = 0;
            if (!ParseUnsigned(argv[++i], 1, 16384, width)) {
                std::cerr << "invalid --width\n";
                return false;
            }
            config.SetSize(width, config.Height());
        } else if (arg == "--height" && i + 1 < argc) {
            unsigned int height = 0;
            if (!ParseUnsigned(argv[++i], 1, 16384, height)) {
                std::cerr << "invalid --height\n";
                return false;
            }
            config.SetSize(config.Width(), height);
        } else {
            std::cerr << "unknown or incomplete option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    ServerConfig config;
    bool validateOnly = false;
    if (!ParseArgs(argc, argv, config, validateOnly)) {
        return 2;
    }
    std::string error;
    if (!config.Validate(&error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    if (validateOnly) {
        return 0;
    }

    MemoryServer server;
    if (!server.Start(config)) {
        std::cerr << "failed to start memory server\n";
        return 1;
    }
    std::cout << "listening on " << config.BindAddress() << ":" << server.Port() << "\n";
    const bool served = server.ServeOne();
    server.Stop();
    return served ? 0 : 1;
}
