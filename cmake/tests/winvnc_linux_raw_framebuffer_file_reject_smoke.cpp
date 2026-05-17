// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxFramebufferSource.h"
#include "vncPortableServerConfig.h"

#include <cassert>
#include <fstream>
#include <string>

using namespace uvnc::winvnc::linuxfb;
using uvnc::winvnc::portable::Framebuffer;
using uvnc::winvnc::portable::ServerConfig;

int main()
{
    Framebuffer framebuffer;
    std::string error;
    assert(!LoadRawFramebufferFile("", 4, 3, ServerConfig::DefaultPixelFormat(), framebuffer, &error));
    assert(error == "raw framebuffer path must not be empty");

    const std::string shortPath = "/tmp/uvnc-raw-framebuffer-short-smoke.bin";
    {
        std::ofstream out(shortPath.c_str(), std::ios::binary);
        out << "short";
    }
    assert(!LoadRawFramebufferFile(shortPath, 4, 3, ServerConfig::DefaultPixelFormat(), framebuffer, &error));
    assert(error == "raw framebuffer file size does not match dimensions and pixel format");

    assert(!LoadRawFramebufferFile("/tmp/uvnc-raw-framebuffer-missing-smoke.bin", 4, 3, ServerConfig::DefaultPixelFormat(), framebuffer, &error));
    assert(error == "cannot stat raw framebuffer file");
    return 0;
}
