// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbUpdate.h"

#include "vncPortableUpdateEncoder.h"

#include <algorithm>
#include <cstring>



namespace {

bool ClipRequestToRect(const uvnc::winvnc::portable::Framebuffer& framebuffer,
                       const uvnc::winvnc::portable::FramebufferUpdateRequest& request,
                       rfb::Rect& rect)
{
    const unsigned int x = std::min(request.x, framebuffer.Width());
    const unsigned int y = std::min(request.y, framebuffer.Height());
    const unsigned int right = std::min(request.x + request.width, framebuffer.Width());
    const unsigned int bottom = std::min(request.y + request.height, framebuffer.Height());
    const unsigned int width = right > x ? right - x : 0;
    const unsigned int height = bottom > y ? bottom - y : 0;
    if (width == 0 || height == 0) {
        return false;
    }
    rect.tl.x = static_cast<int>(x);
    rect.tl.y = static_cast<int>(y);
    rect.br.x = static_cast<int>(right);
    rect.br.y = static_cast<int>(bottom);
    return true;
}

CARD32 SelectFramebufferEncoding(const std::vector<CARD32>& preferredEncodings)
{
    for (std::size_t i = 0; i < preferredEncodings.size(); ++i) {
        if (uvnc::winvnc::portable::UpdateEncoder::SupportsEncoding(preferredEncodings[i])) {
            return preferredEncodings[i];
        }
    }
    return rfbEncodingRaw;
}

bool RectToRequest(const rfb::Rect& rect, uvnc::winvnc::portable::FramebufferUpdateRequest& request)
{
    if (rect.is_empty()) {
        return false;
    }
    request.x = static_cast<unsigned int>(rect.tl.x);
    request.y = static_cast<unsigned int>(rect.tl.y);
    request.width = static_cast<unsigned int>(rect.br.x - rect.tl.x);
    request.height = static_cast<unsigned int>(rect.br.y - rect.tl.y);
    return request.width > 0 && request.height > 0;
}

} // namespace

namespace uvnc {
namespace winvnc {
namespace portable {

std::vector<CARD8> EmptyFramebufferUpdateBytes()
{
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = 0;

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    return bytes;
}

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


std::vector<CARD8> EncodedFramebufferUpdateBytes(const Framebuffer& framebuffer,
                                                 const FramebufferUpdateRequest& request,
                                                 const rfbPixelFormat& remoteFormat,
                                                 const std::vector<CARD32>& preferredEncodings)
{
    rfb::Rect rect;
    if (!ClipRequestToRect(framebuffer, request, rect)) {
        return EmptyFramebufferUpdateBytes();
    }

    const CARD32 encoding = SelectFramebufferEncoding(preferredEncodings);
    std::vector<BYTE> encodedRect;
    UpdateEncoder encoder;
    if (!encoder.EncodeRect(framebuffer, rect, encoding, remoteFormat, encodedRect)) {
        if (!encoder.EncodeRect(framebuffer, rect, rfbEncodingRaw, remoteFormat, encodedRect)) {
            return EmptyFramebufferUpdateBytes();
        }
    }

    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg + encodedRect.size());
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data() + sz_rfbFramebufferUpdateMsg, encodedRect.data(), encodedRect.size());
    return bytes;
}


std::vector<CARD8> EncodedFramebufferUpdateBytes(const Framebuffer& framebuffer,
                                                 const FramebufferUpdateRequest& request,
                                                 const rfb::Region2D& changed,
                                                 const rfbPixelFormat& remoteFormat,
                                                 const std::vector<CARD32>& preferredEncodings)
{
    if (changed.is_empty()) {
        return EmptyFramebufferUpdateBytes();
    }

    rfb::Rect requested;
    if (!ClipRequestToRect(framebuffer, request, requested)) {
        return EmptyFramebufferUpdateBytes();
    }

    rfb::Region2D requestedRegion(requested);
    rfb::Region2D dirty = changed.intersect(requestedRegion);
    if (dirty.is_empty()) {
        return EmptyFramebufferUpdateBytes();
    }

    FramebufferUpdateRequest dirtyRequest = request;
    if (!RectToRequest(dirty.get_bounding_rect(), dirtyRequest)) {
        return EmptyFramebufferUpdateBytes();
    }
    dirtyRequest.incremental = false;
    return EncodedFramebufferUpdateBytes(framebuffer, dirtyRequest, remoteFormat, preferredEncodings);
}

std::vector<CARD8> PointerPositionUpdateBytes(unsigned int x, unsigned int y)
{
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.r.x = Swap16IfLE(static_cast<CARD16>(x));
    header.r.y = Swap16IfLE(static_cast<CARD16>(y));
    header.r.w = 0;
    header.r.h = 0;
    header.encoding = Swap32IfLE(rfbEncodingPointerPos);

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader);
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data() + sz_rfbFramebufferUpdateMsg, &header, sz_rfbFramebufferUpdateRectHeader);
    return bytes;
}

std::vector<CARD8> NewFramebufferSizeUpdateBytes(unsigned int width, unsigned int height)
{
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.r.x = 0;
    header.r.y = 0;
    header.r.w = Swap16IfLE(static_cast<CARD16>(width));
    header.r.h = Swap16IfLE(static_cast<CARD16>(height));
    header.encoding = Swap32IfLE(rfbEncodingNewFBSize);

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader);
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data() + sz_rfbFramebufferUpdateMsg, &header, sz_rfbFramebufferUpdateRectHeader);
    return bytes;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
