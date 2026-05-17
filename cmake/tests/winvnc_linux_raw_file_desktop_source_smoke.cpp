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
    const std::string path = "/tmp/uvnc-raw-file-desktop-source-smoke.bin";
    std::vector<char> pixels(2 * 2 * 4, 0x5a);
    {
        std::ofstream out(path.c_str(), std::ios::binary);
        out.write(pixels.data(), static_cast<std::streamsize>(pixels.size()));
    }

    RawFileDesktopSource source(path, 2, 2, ServerConfig::DefaultPixelFormat());
    assert(source.Size().equals(rfb::Rect(0, 0, 2, 2)));
    assert(source.Format().bitsPerPixel == 32);

    Framebuffer snapshot;
    rfb::Region2D changed;
    assert(source.Snapshot(snapshot, changed));
    assert(snapshot.Width() == 2);
    assert(snapshot.Height() == 2);
    assert(changed.get_bounding_rect().equals(snapshot.Bounds()));
    assert(snapshot.Data()[0] == 0x5a);
    return 0;
}
