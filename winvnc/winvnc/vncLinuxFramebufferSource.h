// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_FRAMEBUFFER_SOURCE_H
#define UVNC_WINVNC_LINUX_FRAMEBUFFER_SOURCE_H

#include "vncPortableDesktopSource.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

bool LoadRawFramebufferFile(const std::string& path,
                            unsigned int width,
                            unsigned int height,
                            const rfbPixelFormat& format,
                            portable::Framebuffer& framebuffer,
                            std::string *error = nullptr);

class RawFileDesktopSource : public portable::DesktopSource {
public:
    RawFileDesktopSource(const std::string& path,
                         unsigned int width,
                         unsigned int height,
                         const rfbPixelFormat& format);

    rfb::Rect Size() const override;
    rfbPixelFormat Format() const override;
    bool Snapshot(portable::Framebuffer& destination, rfb::Region2D& changed) override;

private:
    std::string path_;
    unsigned int width_;
    unsigned int height_;
    rfbPixelFormat format_;
};

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_FRAMEBUFFER_SOURCE_H
