// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbUpdate.h"

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

std::vector<CARD8> RawFramebufferUpdateBytes(const Framebuffer& framebuffer,
                                             const FramebufferUpdateRequest& request)
{
    const unsigned int x = std::min(request.x, framebuffer.Width());
    const unsigned int y = std::min(request.y, framebuffer.Height());
    const unsigned int right = std::min(request.x + request.width, framebuffer.Width());
    const unsigned int bottom = std::min(request.y + request.height, framebuffer.Height());
    const unsigned int width = right > x ? right - x : 0;
    const unsigned int height = bottom > y ? bottom - y : 0;

    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(width > 0 && height > 0 ? 1 : 0);

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    if (width == 0 || height == 0) {
        return bytes;
    }

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.r.x = Swap16IfLE(static_cast<CARD16>(x));
    header.r.y = Swap16IfLE(static_cast<CARD16>(y));
    header.r.w = Swap16IfLE(static_cast<CARD16>(width));
    header.r.h = Swap16IfLE(static_cast<CARD16>(height));
    header.encoding = Swap32IfLE(rfbEncodingRaw);

    const unsigned int bytesPerPixel = framebuffer.BytesPerPixel();
    const std::size_t rowBytes = width * bytesPerPixel;
    const std::size_t pixelBytes = rowBytes * height;
    const std::size_t oldSize = bytes.size();
    bytes.resize(oldSize + sz_rfbFramebufferUpdateRectHeader + pixelBytes);
    std::memcpy(bytes.data() + oldSize, &header, sz_rfbFramebufferUpdateRectHeader);

    CARD8 *dest = bytes.data() + oldSize + sz_rfbFramebufferUpdateRectHeader;
    for (unsigned int row = 0; row < height; ++row) {
        const CARD8 *src = framebuffer.PixelAt(x, y + row);
        std::memcpy(dest + row * rowBytes, src, rowBytes);
    }
    return bytes;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
