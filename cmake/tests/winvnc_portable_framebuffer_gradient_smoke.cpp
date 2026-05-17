// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebufferPattern.h"
#include "vncPortableServerConfig.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    Framebuffer x(4, 3, ServerConfig::DefaultPixelFormat());
    assert(ApplyFramebufferPattern(x, FramebufferPattern::GradientX, 0x00));
    assert(x.PixelAt(0, 0)[0] == 0);
    assert(x.PixelAt(3, 0)[0] == 255);
    assert(x.PixelAt(1, 2)[0] > x.PixelAt(0, 2)[0]);

    Framebuffer y(4, 3, ServerConfig::DefaultPixelFormat());
    assert(ApplyFramebufferPattern(y, FramebufferPattern::GradientY, 0x00));
    assert(y.PixelAt(0, 0)[0] == 0);
    assert(y.PixelAt(0, 2)[0] == 255);
    assert(y.PixelAt(3, 1)[0] > y.PixelAt(3, 0)[0]);
    return 0;
}
