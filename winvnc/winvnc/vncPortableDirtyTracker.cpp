// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDirtyTracker.h"

namespace uvnc {
namespace winvnc {
namespace portable {

DirtyTracker::DirtyTracker(const Framebuffer& framebuffer)
    : framebuffer_(framebuffer), dirty_()
{
}

void DirtyTracker::Clear()
{
    dirty_.clear();
}

bool DirtyTracker::Empty() const
{
    return dirty_.is_empty();
}

void DirtyTracker::MarkDirty(const rfb::Rect& rect)
{
    const rfb::Rect clipped = rect.intersect(framebuffer_.Bounds());
    if (clipped.is_empty()) {
        return;
    }

    rfb::Region2D update(clipped);
    dirty_.assign_union(update);
}

void DirtyTracker::MarkAllDirty()
{
    MarkDirty(framebuffer_.Bounds());
}

rfb::Region2D DirtyTracker::Region() const
{
    return dirty_;
}

rfb::Region2D DirtyTracker::Consume()
{
    rfb::Region2D consumed = dirty_;
    dirty_.clear();
    return consumed;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
