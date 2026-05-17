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

    encoder.SetLocalFormat(format, 16, 16);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "hextile remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 16, 16), "hextile local format should initialize");
    winvnc_test_expect(encoder.NumCodedRects(winvnc_test_rect(0, 0, 16, 16)) == 1, "hextile should report one coded rect");

    const std::uint32_t pixel = 0x10203040;
    std::vector<BYTE> source(16 * 16 * sizeof(pixel));
    for (std::size_t i = 0; i < source.size(); i += sizeof(pixel)) {
        std::memcpy(source.data() + i, &pixel, sizeof(pixel));
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(16, 16));

    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), winvnc_test_rect(0, 0, 16, 16));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader, "hextile encoder should emit payload");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 16, "unexpected hextile width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 16, "unexpected hextile height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingHextile, "unexpected hextile encoding");

    return 0;
}
