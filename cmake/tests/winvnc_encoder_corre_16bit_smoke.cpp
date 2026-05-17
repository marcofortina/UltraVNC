// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencodecorre.h"

#include <cstdint>
#include <cstring>
#include <vector>

int main()
{
    vncEncodeCoRRE encoder;
    rfbPixelFormat format = winvnc_test_true_colour_16();

    encoder.SetLocalFormat(format, 4, 4);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "CoRRE 16-bit remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 4, 4), "CoRRE 16-bit local format should initialize");
    winvnc_test_expect(encoder.NumCodedRects(winvnc_test_rect(0, 0, 4, 4)) == 1, "CoRRE 16-bit rect should report one coded rect");

    const std::uint16_t pixel = 0x39E7;
    std::vector<BYTE> source(4 * 4 * sizeof(pixel));
    for (std::size_t i = 0; i < source.size(); i += sizeof(pixel)) {
        std::memcpy(source.data() + i, &pixel, sizeof(pixel));
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(4, 4));

    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), winvnc_test_rect(0, 0, 4, 4));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader, "CoRRE 16-bit encoder should emit payload");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 4, "unexpected CoRRE 16-bit width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 4, "unexpected CoRRE 16-bit height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingCoRRE, "unexpected CoRRE 16-bit encoding");

    return 0;
}
