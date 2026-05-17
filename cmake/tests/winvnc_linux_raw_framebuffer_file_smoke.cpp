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
#include <vector>

using namespace uvnc::winvnc::linuxfb;
using uvnc::winvnc::portable::Framebuffer;
using uvnc::winvnc::portable::ServerConfig;

int main()
{
    const std::string path = "/tmp/uvnc-raw-framebuffer-file-smoke.bin";
    std::vector<char> pixels(4 * 3 * 4);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<char>(i & 0xff);
    }
    {
        std::ofstream out(path.c_str(), std::ios::binary);
        out.write(pixels.data(), static_cast<std::streamsize>(pixels.size()));
    }

    Framebuffer framebuffer;
    std::string error;
    assert(LoadRawFramebufferFile(path, 4, 3, ServerConfig::DefaultPixelFormat(), framebuffer, &error));
    assert(error.empty());
    assert(framebuffer.Width() == 4);
    assert(framebuffer.Height() == 3);
    assert(framebuffer.SizeBytes() == pixels.size());
    assert(framebuffer.Data()[0] == 0);
    assert(framebuffer.Data()[1] == 1);
    assert(framebuffer.Data()[pixels.size() - 1] == static_cast<unsigned char>((pixels.size() - 1) & 0xff));
    return 0;
}
