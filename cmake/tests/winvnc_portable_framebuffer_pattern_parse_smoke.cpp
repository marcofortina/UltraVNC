// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebufferPattern.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::portable;

int main()
{
    FramebufferPattern pattern = FramebufferPattern::Solid;
    assert(ParseFramebufferPattern("solid", pattern));
    assert(pattern == FramebufferPattern::Solid);
    assert(FramebufferPatternName(pattern) == std::string("solid"));

    assert(ParseFramebufferPattern("checker", pattern));
    assert(pattern == FramebufferPattern::Checker);
    assert(FramebufferPatternName(pattern) == std::string("checker"));

    assert(ParseFramebufferPattern("gradient-x", pattern));
    assert(pattern == FramebufferPattern::GradientX);
    assert(FramebufferPatternName(pattern) == std::string("gradient-x"));

    assert(ParseFramebufferPattern("gradient-y", pattern));
    assert(pattern == FramebufferPattern::GradientY);
    assert(FramebufferPatternName(pattern) == std::string("gradient-y"));

    assert(!ParseFramebufferPattern("unknown", pattern));
    return 0;
}
