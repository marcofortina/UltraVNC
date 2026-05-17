// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebufferPattern.h"

namespace {

BYTE PatternValue(uvnc::winvnc::portable::FramebufferPattern pattern, unsigned int x, unsigned int y, unsigned int width, unsigned int height, BYTE fillByte)
{
    switch (pattern) {
    case uvnc::winvnc::portable::FramebufferPattern::Solid:
        return fillByte;
    case uvnc::winvnc::portable::FramebufferPattern::Checker:
        return ((x / 8 + y / 8) % 2) == 0 ? fillByte : static_cast<BYTE>(~fillByte);
    case uvnc::winvnc::portable::FramebufferPattern::GradientX:
        return width <= 1 ? fillByte : static_cast<BYTE>((x * 255U) / (width - 1));
    case uvnc::winvnc::portable::FramebufferPattern::GradientY:
        return height <= 1 ? fillByte : static_cast<BYTE>((y * 255U) / (height - 1));
    }
    return fillByte;
}

} // namespace

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

bool ApplyFramebufferPattern(Framebuffer& framebuffer, FramebufferPattern pattern, BYTE fillByte)
{
    if (framebuffer.Empty()) {
        return false;
    }

    for (unsigned int y = 0; y < framebuffer.Height(); ++y) {
        for (unsigned int x = 0; x < framebuffer.Width(); ++x) {
            BYTE *pixel = framebuffer.PixelAt(x, y);
            if (!pixel) {
                return false;
            }
            const BYTE value = PatternValue(pattern, x, y, framebuffer.Width(), framebuffer.Height(), fillByte);
            for (unsigned int byte = 0; byte < framebuffer.BytesPerPixel(); ++byte) {
                pixel[byte] = value;
            }
        }
    }
    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
