// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableFramebuffer.h"

#include <cstring>

int main()
{
    using uvnc::winvnc::portable::Framebuffer;

    const rfbPixelFormat format = winvnc_test_true_colour_32();
    Framebuffer source(4, 4, format);
    Framebuffer destination(4, 4, format);

    source.Fill(0x11);
    destination.Fill(0x00);

    BYTE *sourcePixel = source.PixelAt(1, 1);
    winvnc_test_expect(sourcePixel != nullptr, "source pixel lookup failed");
    sourcePixel[0] = 0xaa;
    sourcePixel[1] = 0xbb;
    sourcePixel[2] = 0xcc;
    sourcePixel[3] = 0xdd;

    winvnc_test_expect(destination.CopyRectFrom(source, winvnc_test_rect(1, 1, 2, 2), rfb::Point(2, 2)), "copy rect failed");

    const BYTE *destinationPixel = destination.PixelAt(2, 2);
    winvnc_test_expect(destinationPixel != nullptr, "destination pixel lookup failed");
    winvnc_test_expect(std::memcmp(sourcePixel, destinationPixel, 4) == 0, "copied pixel mismatch");
    winvnc_test_expect(!destination.CopyRectFrom(source, winvnc_test_rect(3, 3, 5, 5), rfb::Point(0, 0)), "out-of-bounds copy succeeded");

    return 0;
}
