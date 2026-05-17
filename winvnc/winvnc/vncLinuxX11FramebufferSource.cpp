// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"

namespace uvnc {
namespace winvnc {
namespace linuxfb {

X11DesktopSource::X11DesktopSource(unsigned int width, unsigned int height, const rfbPixelFormat& format)
    : width_(width),
      height_(height),
      format_(format)
{
}

rfb::Rect X11DesktopSource::Size() const
{
    return rfb::Rect(0, 0, static_cast<int>(width_), static_cast<int>(height_));
}

rfbPixelFormat X11DesktopSource::Format() const
{
    return format_;
}

bool X11DesktopSource::Snapshot(portable::Framebuffer& destination, rfb::Region2D& changed)
{
    destination.Clear();
    changed.clear();
    return false;
}

bool X11DesktopSource::IsAvailable()
{
    return false;
}

const char *X11DesktopSource::UnavailableReason()
{
    return "X11 capture backend is not built in this native Linux subset yet";
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
