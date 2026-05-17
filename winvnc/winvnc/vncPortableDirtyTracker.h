// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_DIRTY_TRACKER_H
#define UVNC_WINVNC_PORTABLE_DIRTY_TRACKER_H

#include "rfbRegion.h"
#include "vncPortableFramebuffer.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class DirtyTracker {
public:
    explicit DirtyTracker(const Framebuffer& framebuffer);

    void Clear();
    bool Empty() const;
    void MarkDirty(const rfb::Rect& rect);
    void MarkAllDirty();

    rfb::Region2D Region() const;
    rfb::Region2D Consume();

private:
    const Framebuffer& framebuffer_;
    rfb::Region2D dirty_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_DIRTY_TRACKER_H
