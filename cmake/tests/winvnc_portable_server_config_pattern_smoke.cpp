// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    assert(config.Pattern() == FramebufferPattern::Solid);
    config.SetPattern(FramebufferPattern::Checker);
    assert(config.Pattern() == FramebufferPattern::Checker);
    config.SetPattern(FramebufferPattern::GradientX);
    assert(config.Pattern() == FramebufferPattern::GradientX);
    return 0;
}
