// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableDesktopSource.h"

#include <cstring>

int main()
{
    using uvnc::winvnc::portable::Framebuffer;
    using uvnc::winvnc::portable::MemoryDesktopSource;

    Framebuffer source(3, 2, winvnc_test_true_colour_32());
    source.Fill(0x42);

    MemoryDesktopSource desktop(source);
    Framebuffer snapshot;
    rfb::Region2D changed;

    winvnc_test_expect(desktop.Size().equals(source.Bounds()), "desktop source size mismatch");
    winvnc_test_expect(desktop.Snapshot(snapshot, changed), "desktop source snapshot failed");
    winvnc_test_expect(snapshot.SizeBytes() == source.SizeBytes(), "snapshot size mismatch");
    winvnc_test_expect(std::memcmp(snapshot.Data(), source.Data(), source.SizeBytes()) == 0, "snapshot data mismatch");
    winvnc_test_expect(changed.get_bounding_rect().equals(source.Bounds()), "initial snapshot should mark all dirty");

    rfb::Region2D changedAgain;
    winvnc_test_expect(desktop.Snapshot(snapshot, changedAgain), "second desktop source snapshot failed");
    winvnc_test_expect(changedAgain.is_empty(), "second snapshot should consume dirty region");

    return 0;
}
