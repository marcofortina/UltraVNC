// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableFramebufferDiff.h"

int main()
{
    using uvnc::winvnc::portable::Framebuffer;
    using uvnc::winvnc::portable::FramebufferDiff;

    Framebuffer previous(4, 4, winvnc_test_true_colour_32());
    Framebuffer current(4, 4, winvnc_test_true_colour_32());
    previous.Fill(0x00);
    current.Fill(0x00);

    winvnc_test_expect(FramebufferDiff::Compatible(previous, current), "compatible framebuffers were rejected");
    winvnc_test_expect(FramebufferDiff::FindChangedRows(previous, current).is_empty(), "unchanged framebuffers produced dirty rows");

    BYTE *pixel = current.PixelAt(2, 1);
    winvnc_test_expect(pixel != nullptr, "pixel lookup failed");
    pixel[0] = 0xff;

    const rfb::Region2D changed = FramebufferDiff::FindChangedRows(previous, current);
    winvnc_test_expect(changed.get_bounding_rect().equals(winvnc_test_rect(0, 1, 4, 2)), "unexpected changed row bounds");

    Framebuffer resized(5, 4, winvnc_test_true_colour_32());
    winvnc_test_expect(!FramebufferDiff::Compatible(previous, resized), "different size framebuffer was treated as compatible");
    winvnc_test_expect(FramebufferDiff::FindChangedRows(previous, resized).get_bounding_rect().equals(resized.Bounds()), "incompatible framebuffer should mark all current bounds dirty");

    return 0;
}
