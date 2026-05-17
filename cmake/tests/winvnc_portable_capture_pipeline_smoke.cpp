// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableCapturePipeline.h"

int main()
{
    using uvnc::winvnc::portable::CapturePipeline;
    using uvnc::winvnc::portable::Framebuffer;
    using uvnc::winvnc::portable::MemoryDesktopSource;

    Framebuffer frame(4, 4, winvnc_test_true_colour_32());
    frame.Fill(0x00);

    MemoryDesktopSource source(frame);
    CapturePipeline pipeline(source);
    rfb::Region2D changed;

    winvnc_test_expect(!pipeline.HasFrame(), "pipeline should start without a frame");
    winvnc_test_expect(pipeline.Capture(changed), "initial capture failed");
    winvnc_test_expect(pipeline.HasFrame(), "pipeline did not record first frame");
    winvnc_test_expect(changed.get_bounding_rect().equals(frame.Bounds()), "initial capture should mark all dirty");

    BYTE *pixel = source.MutableFramebuffer().PixelAt(1, 2);
    winvnc_test_expect(pixel != nullptr, "mutable pixel lookup failed");
    pixel[0] = 0x7f;
    source.MarkDirty(winvnc_test_rect(1, 2, 2, 3));

    winvnc_test_expect(pipeline.Capture(changed), "second capture failed");
    winvnc_test_expect(changed.get_bounding_rect().equals(winvnc_test_rect(0, 2, 4, 3)), "changed row should include modified row");

    return 0;
}
