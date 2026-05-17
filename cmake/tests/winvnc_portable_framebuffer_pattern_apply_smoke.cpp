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
    Framebuffer solid(4, 4, ServerConfig::DefaultPixelFormat());
    assert(ApplyFramebufferPattern(solid, FramebufferPattern::Solid, 0x5a));
    for (std::size_t i = 0; i < solid.SizeBytes(); ++i) {
        assert(solid.Data()[i] == 0x5a);
    }

    Framebuffer checker(16, 8, ServerConfig::DefaultPixelFormat());
    assert(ApplyFramebufferPattern(checker, FramebufferPattern::Checker, 0x11));
    assert(checker.PixelAt(0, 0)[0] == 0x11);
    assert(checker.PixelAt(8, 0)[0] == static_cast<BYTE>(~0x11));
    assert(checker.PixelAt(0, 7)[0] == 0x11);
    return 0;
}
