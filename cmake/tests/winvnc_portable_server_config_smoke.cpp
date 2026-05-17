// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <iostream>
#include <string>

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
    return 0;
}
