// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbUpdate.h"

#include <cassert>
#include <cstring>

using uvnc::winvnc::portable::Framebuffer;
using uvnc::winvnc::portable::FramebufferUpdateRequest;
using uvnc::winvnc::portable::RawFramebufferUpdateBytes;

int main()
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.trueColour = 1;
    format.redMax = 255;
    format.greenMax = 255;
    format.blueMax = 255;

    Framebuffer framebuffer(4, 3, format);
    for (unsigned int y = 0; y < framebuffer.Height(); ++y) {
        for (unsigned int x = 0; x < framebuffer.Width(); ++x) {
            BYTE *pixel = framebuffer.PixelAt(x, y);
            pixel[0] = static_cast<BYTE>(x);
            pixel[1] = static_cast<BYTE>(y);
            pixel[2] = 7;
            pixel[3] = 255;
        }
    }

    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 1;
    request.y = 1;
    request.width = 2;
    request.height = 1;

    const std::vector<CARD8> bytes = RawFramebufferUpdateBytes(framebuffer, request);
    assert(bytes.size() == sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader + 2 * 4);

    rfbFramebufferUpdateMsg update;
    std::memcpy(&update, bytes.data(), sizeof(update));
    assert(update.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(update.nRects) == 1);

    rfbFramebufferUpdateRectHeader header;
    std::memcpy(&header, bytes.data() + sz_rfbFramebufferUpdateMsg, sizeof(header));
    assert(Swap16IfLE(header.r.x) == 1);
    assert(Swap16IfLE(header.r.y) == 1);
    assert(Swap16IfLE(header.r.w) == 2);
    assert(Swap16IfLE(header.r.h) == 1);
    assert(Swap32IfLE(header.encoding) == rfbEncodingRaw);

    const std::size_t payload = sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader;
    assert(bytes[payload] == 1);
    assert(bytes[payload + 1] == 1);
    assert(bytes[payload + 4] == 2);
    assert(bytes[payload + 5] == 1);

    request.x = 100;
    const std::vector<CARD8> empty = RawFramebufferUpdateBytes(framebuffer, request);
    std::memcpy(&update, empty.data(), sizeof(update));
    assert(Swap16IfLE(update.nRects) == 0);

    return 0;
}
