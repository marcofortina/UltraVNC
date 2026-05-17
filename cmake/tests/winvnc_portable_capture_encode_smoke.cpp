// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableCapturePipeline.h"
#include "vncPortableUpdateEncoder.h"

#include <vector>

int main()
{
    using uvnc::winvnc::portable::CapturePipeline;
    using uvnc::winvnc::portable::Framebuffer;
    using uvnc::winvnc::portable::MemoryDesktopSource;
    using uvnc::winvnc::portable::UpdateEncoder;

    Framebuffer frame(3, 3, winvnc_test_true_colour_32());
    frame.Fill(0x5a);

    MemoryDesktopSource source(frame);
    CapturePipeline pipeline(source);
    rfb::Region2D changed;
    winvnc_test_expect(pipeline.Capture(changed), "capture failed");

    UpdateEncoder encoder;
    winvnc_test_expect(encoder.Initialize(pipeline.Current().Format(), pipeline.Current().Width(), pipeline.Current().Height()), "encoder initialization failed");

    std::vector<BYTE> encoded;
    winvnc_test_expect(encoder.EncodeRawRect(pipeline.Current(), changed.get_bounding_rect(), encoded), "capture encode failed");
    winvnc_test_expect(encoded.size() == sz_rfbFramebufferUpdateRectHeader + pipeline.Current().SizeBytes(), "capture encode size mismatch");

    return 0;
}
