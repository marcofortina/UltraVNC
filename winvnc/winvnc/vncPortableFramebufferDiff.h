// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_FRAMEBUFFER_DIFF_H
#define UVNC_WINVNC_PORTABLE_FRAMEBUFFER_DIFF_H

#include "rfbRegion.h"
#include "vncPortableFramebuffer.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class FramebufferDiff {
public:
    static bool Compatible(const Framebuffer& previous, const Framebuffer& current);
    static rfb::Region2D FindChangedRows(const Framebuffer& previous, const Framebuffer& current);
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_FRAMEBUFFER_DIFF_H
