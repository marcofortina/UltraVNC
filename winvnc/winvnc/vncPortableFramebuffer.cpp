// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebuffer.h"

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

Framebuffer::Framebuffer()
    : width_(0), height_(0), bytesPerPixel_(0), stride_(0), format_(), pixels_()
{
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height, const rfbPixelFormat& format)
    : Framebuffer()
{
    Reset(width, height, format);
}

bool Framebuffer::Reset(unsigned int width, unsigned int height, const rfbPixelFormat& format)
{
    if (width == 0 || height == 0 || format.bitsPerPixel == 0 || (format.bitsPerPixel % 8) != 0) {
        Clear();
        return false;
    }

    const unsigned int bytesPerPixel = format.bitsPerPixel / 8;
    const std::size_t stride = static_cast<std::size_t>(width) * bytesPerPixel;
    const std::size_t size = stride * height;

    if (stride > static_cast<std::size_t>(~0U) || size == 0) {
        Clear();
        return false;
    }

    width_ = width;
    height_ = height;
    bytesPerPixel_ = bytesPerPixel;
    stride_ = static_cast<unsigned int>(stride);
    format_ = format;
    pixels_.assign(size, 0);
    return true;
}

void Framebuffer::Clear()
{
    width_ = 0;
    height_ = 0;
    bytesPerPixel_ = 0;
    stride_ = 0;
    format_ = rfbPixelFormat();
    pixels_.clear();
}

void Framebuffer::Fill(BYTE value)
{
    std::fill(pixels_.begin(), pixels_.end(), value);
}

rfb::Rect Framebuffer::Bounds() const
{
    return rfb::Rect(0, 0, static_cast<int>(width_), static_cast<int>(height_));
}

BYTE *Framebuffer::Data()
{
    return pixels_.empty() ? nullptr : pixels_.data();
}

const BYTE *Framebuffer::Data() const
{
    return pixels_.empty() ? nullptr : pixels_.data();
}

BYTE *Framebuffer::PixelAt(unsigned int x, unsigned int y)
{
    if (x >= width_ || y >= height_) {
        return nullptr;
    }
    return Data() + static_cast<std::size_t>(y) * stride_ + static_cast<std::size_t>(x) * bytesPerPixel_;
}

const BYTE *Framebuffer::PixelAt(unsigned int x, unsigned int y) const
{
    if (x >= width_ || y >= height_) {
        return nullptr;
    }
    return Data() + static_cast<std::size_t>(y) * stride_ + static_cast<std::size_t>(x) * bytesPerPixel_;
}

bool Framebuffer::Contains(const rfb::Rect& rect) const
{
    return !rect.is_empty() && rect.enclosed_by(Bounds());
}

bool Framebuffer::CopyRectFrom(const Framebuffer& source, const rfb::Rect& sourceRect, const rfb::Point& destination)
{
    if (bytesPerPixel_ == 0 || bytesPerPixel_ != source.bytesPerPixel_ || sourceRect.is_empty()) {
        return false;
    }

    const rfb::Rect destRect(destination.x, destination.y,
                             destination.x + sourceRect.width(),
                             destination.y + sourceRect.height());
    if (!source.Contains(sourceRect) || !Contains(destRect)) {
        return false;
    }

    const std::size_t rowBytes = static_cast<std::size_t>(sourceRect.width()) * bytesPerPixel_;
    for (int row = 0; row < sourceRect.height(); ++row) {
        const BYTE *src = source.PixelAt(static_cast<unsigned int>(sourceRect.tl.x), static_cast<unsigned int>(sourceRect.tl.y + row));
        BYTE *dst = PixelAt(static_cast<unsigned int>(destination.x), static_cast<unsigned int>(destination.y + row));
        std::memmove(dst, src, rowBytes);
    }

    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
