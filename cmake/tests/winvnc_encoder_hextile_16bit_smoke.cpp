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
    rfbPixelFormat format = winvnc_test_true_colour_16();

    encoder.SetLocalFormat(format, 16, 16);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "hextile 16-bit remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 16, 16), "hextile 16-bit local format should initialize");

    const std::uint16_t pixel = 0x7BEF;
    std::vector<BYTE> source(16 * 16 * sizeof(pixel));
    for (std::size_t i = 0; i < source.size(); i += sizeof(pixel)) {
        std::memcpy(source.data() + i, &pixel, sizeof(pixel));
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(16, 16));

    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), winvnc_test_rect(0, 0, 16, 16));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader, "hextile 16-bit encoder should emit payload");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 16, "unexpected hextile 16-bit width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 16, "unexpected hextile 16-bit height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingHextile, "unexpected hextile 16-bit encoding");

    return 0;
}
