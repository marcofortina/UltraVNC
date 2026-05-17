// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"
#include "vncPortableServerConfig.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxfb;
using namespace uvnc::winvnc::portable;

int main()
{
    X11DesktopSource source(640, 480, ServerConfig::DefaultPixelFormat());
    assert(source.Size().equals(rfb::Rect(0, 0, 640, 480)));
    assert(source.Format().bitsPerPixel == 32);
    assert(!X11DesktopSource::IsAvailable());
    assert(std::string(X11DesktopSource::UnavailableReason()).find("X11") != std::string::npos);

    Framebuffer snapshot;
    rfb::Region2D changed;
    assert(!source.Snapshot(snapshot, changed));
    assert(snapshot.Empty());
    assert(changed.is_empty());
    return 0;
}
