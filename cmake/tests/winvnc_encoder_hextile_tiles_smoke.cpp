// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencodehext.h"

#include <cstdint>
#include <cstring>
#include <vector>

int main()
{
    vncEncodeHexT encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 32, 16);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "hextile tiled remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 32, 16), "hextile tiled local format should initialize");
    winvnc_test_expect(encoder.NumCodedRects(winvnc_test_rect(0, 0, 32, 16)) == 1, "hextile tiled rect should report one coded rect");

    std::vector<BYTE> source(32 * 16 * sizeof(std::uint32_t));
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 32; ++x) {
            const std::uint32_t pixel = x < 16 ? 0x10101010 : 0x20202020;
            const std::size_t offset = static_cast<std::size_t>(y * 32 + x) * sizeof(pixel);
            std::memcpy(source.data() + offset, &pixel, sizeof(pixel));
        }
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(32, 16));

    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), winvnc_test_rect(0, 0, 32, 16));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader, "hextile tiled encoder should emit payload");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 32, "unexpected hextile tiled width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 16, "unexpected hextile tiled height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingHextile, "unexpected hextile tiled encoding");

    return 0;
}
