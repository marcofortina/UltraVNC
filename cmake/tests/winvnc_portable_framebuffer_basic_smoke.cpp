// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableFramebuffer.h"

int main()
{
    uvnc::winvnc::portable::Framebuffer framebuffer;
    const rfbPixelFormat format = winvnc_test_true_colour_32();

    winvnc_test_expect(framebuffer.Reset(4, 3, format), "framebuffer reset failed");
    winvnc_test_expect(framebuffer.Width() == 4, "unexpected framebuffer width");
    winvnc_test_expect(framebuffer.Height() == 3, "unexpected framebuffer height");
    winvnc_test_expect(framebuffer.BytesPerPixel() == 4, "unexpected bytes per pixel");
    winvnc_test_expect(framebuffer.Stride() == 16, "unexpected framebuffer stride");
    winvnc_test_expect(framebuffer.SizeBytes() == 48, "unexpected framebuffer byte size");
    winvnc_test_expect(framebuffer.Bounds().equals(winvnc_test_rect(0, 0, 4, 3)), "unexpected framebuffer bounds");
    winvnc_test_expect(framebuffer.PixelAt(3, 2) != nullptr, "valid pixel lookup failed");
    winvnc_test_expect(framebuffer.PixelAt(4, 2) == nullptr, "out-of-range pixel lookup succeeded");

    return 0;
}
