// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H
#define UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H

#include "rfb.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class ServerConfig {
public:
    ServerConfig();

    const std::string& BindAddress() const { return bindAddress_; }
    unsigned short Port() const { return port_; }
    unsigned int Width() const { return width_; }
    unsigned int Height() const { return height_; }
    const std::string& DesktopName() const { return desktopName_; }
    rfbPixelFormat PixelFormat() const { return format_; }

    void SetBindAddress(const std::string& bindAddress) { bindAddress_ = bindAddress; }
    void SetPort(unsigned short port) { port_ = port; }
    void SetSize(unsigned int width, unsigned int height);
    void SetDesktopName(const std::string& desktopName) { desktopName_ = desktopName; }
    void SetPixelFormat(const rfbPixelFormat& format) { format_ = format; }

    bool Validate(std::string *error = nullptr) const;

    static rfbPixelFormat DefaultPixelFormat();

private:
    std::string bindAddress_;
    unsigned short port_;
    unsigned int width_;
    unsigned int height_;
    std::string desktopName_;
    rfbPixelFormat format_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H
