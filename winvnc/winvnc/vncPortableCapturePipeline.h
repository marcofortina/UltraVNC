// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_CAPTURE_PIPELINE_H
#define UVNC_WINVNC_PORTABLE_CAPTURE_PIPELINE_H

#include "vncPortableDesktopSource.h"
#include "vncPortableFramebufferDiff.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class CapturePipeline {
public:
    explicit CapturePipeline(DesktopSource& source);

    bool Capture(rfb::Region2D& changed);
    const Framebuffer& Current() const { return current_; }
    const Framebuffer& Previous() const { return previous_; }
    bool HasFrame() const { return hasFrame_; }

private:
    DesktopSource& source_;
    Framebuffer current_;
    Framebuffer previous_;
    bool hasFrame_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_CAPTURE_PIPELINE_H
