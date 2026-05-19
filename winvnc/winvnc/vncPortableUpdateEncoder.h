// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_UPDATE_ENCODER_H
#define UVNC_WINVNC_PORTABLE_UPDATE_ENCODER_H

#include "vncPortableFramebuffer.h"
#include "vncencoder.h"

#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

class UpdateEncoder {
public:
    UpdateEncoder();

    bool Initialize(const rfbPixelFormat& format, unsigned int width, unsigned int height);
    bool Initialize(const rfbPixelFormat& localFormat, const rfbPixelFormat& remoteFormat, unsigned int width, unsigned int height);
    bool EncodeRawRect(const Framebuffer& framebuffer, const rfb::Rect& rect, std::vector<BYTE>& encoded);
    bool EncodeRect(const Framebuffer& framebuffer, const rfb::Rect& rect, CARD32 encoding, const rfbPixelFormat& remoteFormat, std::vector<BYTE>& encoded);

    static bool SupportsEncoding(CARD32 encoding);

private:
    vncEncoder encoder_;
    bool initialized_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_UPDATE_ENCODER_H
