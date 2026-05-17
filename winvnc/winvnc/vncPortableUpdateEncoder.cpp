// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableUpdateEncoder.h"

namespace uvnc {
namespace winvnc {
namespace portable {

UpdateEncoder::UpdateEncoder()
    : encoder_(), initialized_(false)
{
}

bool UpdateEncoder::Initialize(const rfbPixelFormat& format, unsigned int width, unsigned int height)
{
    rfbPixelFormat remoteFormat = format;
    rfbPixelFormat localFormat = format;

    encoder_.SetLocalFormat(localFormat, static_cast<int>(width), static_cast<int>(height));
    if (!encoder_.SetRemoteFormat(remoteFormat)) {
        initialized_ = false;
        return false;
    }

    initialized_ = encoder_.SetLocalFormat(localFormat, static_cast<int>(width), static_cast<int>(height));
    return initialized_;
}

bool UpdateEncoder::EncodeRawRect(const Framebuffer& framebuffer, const rfb::Rect& rect, std::vector<BYTE>& encoded)
{
    if (!initialized_ || framebuffer.Empty() || !framebuffer.Contains(rect)) {
        return false;
    }

    encoded.assign(encoder_.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
    const UINT encodedSize = encoder_.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), encoded.data(), rect);
    if (encodedSize == 0 || encodedSize > encoded.size()) {
        encoded.clear();
        return false;
    }

    encoded.resize(encodedSize);
    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
