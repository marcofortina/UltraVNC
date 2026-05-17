// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebufferDiff.h"

#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

bool FramebufferDiff::Compatible(const Framebuffer& previous, const Framebuffer& current)
{
    return previous.Width() == current.Width() &&
           previous.Height() == current.Height() &&
           previous.BytesPerPixel() == current.BytesPerPixel() &&
           previous.Stride() == current.Stride() &&
           previous.SizeBytes() == current.SizeBytes();
}

rfb::Region2D FramebufferDiff::FindChangedRows(const Framebuffer& previous, const Framebuffer& current)
{
    rfb::Region2D changed;
    if (!Compatible(previous, current) || previous.Empty()) {
        changed.reset(current.Bounds());
        return changed;
    }

    for (unsigned int y = 0; y < current.Height(); ++y) {
        const BYTE *previousRow = previous.PixelAt(0, y);
        const BYTE *currentRow = current.PixelAt(0, y);
        if (std::memcmp(previousRow, currentRow, current.Stride()) != 0) {
            rfb::Region2D row(rfb::Rect(0, static_cast<int>(y), static_cast<int>(current.Width()), static_cast<int>(y + 1)));
            changed.assign_union(row);
        }
    }

    return changed;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
