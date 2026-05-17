// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableDirtyTracker.h"
#include "vncPortableFramebuffer.h"

#include <vector>

int main()
{
    uvnc::winvnc::portable::Framebuffer framebuffer(10, 8, winvnc_test_true_colour_32());
    uvnc::winvnc::portable::DirtyTracker tracker(framebuffer);

    winvnc_test_expect(tracker.Empty(), "dirty tracker should start empty");
    tracker.MarkDirty(winvnc_test_rect(2, 3, 6, 7));
    winvnc_test_expect(!tracker.Empty(), "dirty tracker did not record update");
    winvnc_test_expect(tracker.Region().get_bounding_rect().equals(winvnc_test_rect(2, 3, 6, 7)), "unexpected dirty bounds");

    tracker.MarkDirty(winvnc_test_rect(-10, -10, 2, 2));
    winvnc_test_expect(tracker.Region().get_bounding_rect().equals(winvnc_test_rect(0, 0, 6, 7)), "dirty tracker did not clip to framebuffer");

    rfb::Region2D consumed = tracker.Consume();
    winvnc_test_expect(!consumed.is_empty(), "consumed dirty region was empty");
    winvnc_test_expect(tracker.Empty(), "dirty tracker did not clear after consume");

    tracker.MarkAllDirty();
    winvnc_test_expect(tracker.Region().get_bounding_rect().equals(framebuffer.Bounds()), "mark all dirty did not cover framebuffer");

    return 0;
}
