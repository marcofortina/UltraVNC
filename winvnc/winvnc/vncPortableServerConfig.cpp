// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

ServerConfig::ServerConfig()
    : bindAddress_("127.0.0.1"),
      port_(0),
      width_(640),
      height_(480),
      desktopName_("UltraVNC native Linux memory server"),
      fillByte_(0x22),
      format_(DefaultPixelFormat())
{
}

void ServerConfig::SetSize(unsigned int width, unsigned int height)
{
    width_ = width;
    height_ = height;
}

bool ServerConfig::Validate(std::string *error) const
{
    if (bindAddress_.empty()) {
        if (error) *error = "bind address must not be empty";
        return false;
    }
    if (width_ == 0 || height_ == 0) {
        if (error) *error = "framebuffer size must be non-zero";
        return false;
    }
    if (width_ > 16384 || height_ > 16384) {
        if (error) *error = "framebuffer size is too large";
        return false;
    }
    if (desktopName_.empty()) {
        if (error) *error = "desktop name must not be empty";
        return false;
    }
    if (format_.bitsPerPixel != 8 && format_.bitsPerPixel != 16 && format_.bitsPerPixel != 32) {
        if (error) *error = "bitsPerPixel must be 8, 16, or 32";
        return false;
    }
    if (format_.bitsPerPixel % 8 != 0) {
        if (error) *error = "bitsPerPixel must be byte-aligned";
        return false;
    }
    return true;
}

rfbPixelFormat ServerConfig::DefaultPixelFormat()
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.bigEndian = 0;
    format.trueColour = 1;
    format.redMax = 255;
    format.greenMax = 255;
    format.blueMax = 255;
    format.redShift = 16;
    format.greenShift = 8;
    format.blueShift = 0;
    return format;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
