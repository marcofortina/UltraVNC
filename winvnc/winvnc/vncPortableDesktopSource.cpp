// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDesktopSource.h"

namespace uvnc {
namespace winvnc {
namespace portable {

MemoryDesktopSource::MemoryDesktopSource(const Framebuffer& framebuffer)
    : framebuffer_(framebuffer), dirty_(framebuffer_)
{
    dirty_.MarkAllDirty();
}

rfb::Rect MemoryDesktopSource::Size() const
{
    return framebuffer_.Bounds();
}

rfbPixelFormat MemoryDesktopSource::Format() const
{
    return framebuffer_.Format();
}

bool MemoryDesktopSource::Snapshot(Framebuffer& destination, rfb::Region2D& changed)
{
    if (!destination.Reset(framebuffer_.Width(), framebuffer_.Height(), framebuffer_.Format())) {
        return false;
    }

    if (!destination.CopyRectFrom(framebuffer_, framebuffer_.Bounds(), rfb::Point(0, 0))) {
        return false;
    }

    changed = dirty_.Consume();
    return true;
}

Framebuffer& MemoryDesktopSource::MutableFramebuffer()
{
    return framebuffer_;
}

const Framebuffer& MemoryDesktopSource::CurrentFramebuffer() const
{
    return framebuffer_;
}

void MemoryDesktopSource::MarkDirty(const rfb::Rect& rect)
{
    dirty_.MarkDirty(rect);
}

void MemoryDesktopSource::MarkAllDirty()
{
    dirty_.MarkAllDirty();
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
