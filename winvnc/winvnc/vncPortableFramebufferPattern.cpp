// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebufferPattern.h"

namespace uvnc {
namespace winvnc {
namespace portable {

const char *FramebufferPatternName(FramebufferPattern pattern)
{
    switch (pattern) {
    case FramebufferPattern::Solid:
        return "solid";
    case FramebufferPattern::Checker:
        return "checker";
    case FramebufferPattern::GradientX:
        return "gradient-x";
    case FramebufferPattern::GradientY:
        return "gradient-y";
    }
    return "solid";
}

bool ParseFramebufferPattern(const std::string& value, FramebufferPattern& pattern)
{
    if (value == "solid") {
        pattern = FramebufferPattern::Solid;
        return true;
    }
    if (value == "checker") {
        pattern = FramebufferPattern::Checker;
        return true;
    }
    if (value == "gradient-x") {
        pattern = FramebufferPattern::GradientX;
        return true;
    }
    if (value == "gradient-y") {
        pattern = FramebufferPattern::GradientY;
        return true;
    }
    return false;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
