// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableCapturePipeline.h"

namespace uvnc {
namespace winvnc {
namespace portable {

CapturePipeline::CapturePipeline(DesktopSource& source)
    : source_(source), current_(), previous_(), hasFrame_(false)
{
}

bool CapturePipeline::Capture(rfb::Region2D& changed)
{
    Framebuffer next;
    rfb::Region2D sourceChanged;
    if (!source_.Snapshot(next, sourceChanged)) {
        return false;
    }

    if (!hasFrame_) {
        changed.reset(next.Bounds());
    } else {
        changed = FramebufferDiff::FindChangedRows(current_, next);
        changed.assign_union(sourceChanged);
    }

    previous_ = current_;
    current_ = next;
    hasFrame_ = true;
    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
