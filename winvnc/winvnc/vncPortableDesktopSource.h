// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_DESKTOP_SOURCE_H
#define UVNC_WINVNC_PORTABLE_DESKTOP_SOURCE_H

#include "vncPortableDirtyTracker.h"
#include "vncPortableFramebuffer.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class DesktopSource {
public:
    virtual ~DesktopSource() {}

    virtual rfb::Rect Size() const = 0;
    virtual rfbPixelFormat Format() const = 0;
    virtual bool Snapshot(Framebuffer& destination, rfb::Region2D& changed) = 0;
};

class MemoryDesktopSource : public DesktopSource {
public:
    explicit MemoryDesktopSource(const Framebuffer& framebuffer);

    rfb::Rect Size() const override;
    rfbPixelFormat Format() const override;
    bool Snapshot(Framebuffer& destination, rfb::Region2D& changed) override;

    Framebuffer& MutableFramebuffer();
    const Framebuffer& CurrentFramebuffer() const;
    void MarkDirty(const rfb::Rect& rect);
    void MarkAllDirty();

private:
    Framebuffer framebuffer_;
    DirtyTracker dirty_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_DESKTOP_SOURCE_H
