// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_FRAMEBUFFER_PATTERN_H
#define UVNC_WINVNC_PORTABLE_FRAMEBUFFER_PATTERN_H

#include "vncPortableFramebuffer.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

enum class FramebufferPattern {
    Solid,
    Checker,
    GradientX,
    GradientY
};

const char *FramebufferPatternName(FramebufferPattern pattern);
bool ParseFramebufferPattern(const std::string& value, FramebufferPattern& pattern);
bool ApplyFramebufferPattern(Framebuffer& framebuffer, FramebufferPattern pattern, BYTE fillByte);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_FRAMEBUFFER_PATTERN_H
