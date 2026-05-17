// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_FRAMEBUFFER_H
#define UVNC_WINVNC_PORTABLE_FRAMEBUFFER_H

#include "rfb.h"
#include "rfbRect.h"
#include "stdhdrs.h"

#include <cstddef>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

class Framebuffer {
public:
    Framebuffer();
    Framebuffer(unsigned int width, unsigned int height, const rfbPixelFormat& format);

    bool Reset(unsigned int width, unsigned int height, const rfbPixelFormat& format);
    void Clear();
    void Fill(BYTE value);

    unsigned int Width() const { return width_; }
    unsigned int Height() const { return height_; }
    unsigned int BytesPerPixel() const { return bytesPerPixel_; }
    unsigned int Stride() const { return stride_; }
    std::size_t SizeBytes() const { return pixels_.size(); }
    bool Empty() const { return pixels_.empty(); }
    rfbPixelFormat Format() const { return format_; }
    rfb::Rect Bounds() const;

    BYTE *Data();
    const BYTE *Data() const;
    BYTE *PixelAt(unsigned int x, unsigned int y);
    const BYTE *PixelAt(unsigned int x, unsigned int y) const;

    bool Contains(const rfb::Rect& rect) const;
    bool CopyRectFrom(const Framebuffer& source, const rfb::Rect& sourceRect, const rfb::Point& destination);

private:
    unsigned int width_;
    unsigned int height_;
    unsigned int bytesPerPixel_;
    unsigned int stride_;
    rfbPixelFormat format_;
    std::vector<BYTE> pixels_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_FRAMEBUFFER_H
