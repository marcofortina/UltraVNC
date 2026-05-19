// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_X11_FRAMEBUFFER_SOURCE_H
#define UVNC_WINVNC_LINUX_X11_FRAMEBUFFER_SOURCE_H

#include "vncPortableDesktopSource.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

class X11DesktopSource : public portable::DesktopSource {
public:
    explicit X11DesktopSource(const std::string& displayName = std::string());
    X11DesktopSource(unsigned int width, unsigned int height, const rfbPixelFormat& format);
    ~X11DesktopSource() override;

    rfb::Rect Size() const override;
    rfbPixelFormat Format() const override;
    bool Snapshot(portable::Framebuffer& destination, rfb::Region2D& changed) override;
    const std::string& LastError() const { return lastError_; }

    static bool IsBuildAvailable();
    static bool IsAvailable(const std::string& displayName = std::string());
    static bool IsXShmBuildAvailable();
    static bool IsXShmRuntimeAvailable(const std::string& displayName = std::string());
    static bool IsXDamageBuildAvailable();
    static bool IsXDamageRuntimeAvailable(const std::string& displayName = std::string());
    static const char *UnavailableReason();

private:
    bool Initialize(std::string *error = nullptr);
    void RefineChangedRegion(portable::Framebuffer& destination, rfb::Region2D& changed);
    rfb::Region2D DrainDamageRegion();

    std::string displayName_;
    unsigned int width_;
    unsigned int height_;
    rfbPixelFormat format_;
    void *display_;
    unsigned long root_;
    int screen_;
    bool initialized_;
    bool damageAvailable_;
    unsigned long damage_;
    int damageEventBase_;
    int damageErrorBase_;
    std::string lastError_;
    portable::Framebuffer previousFrame_;
};

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_X11_FRAMEBUFFER_SOURCE_H
