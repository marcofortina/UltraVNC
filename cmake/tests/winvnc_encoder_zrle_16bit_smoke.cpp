// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencodezrle.h"

#include <cstdint>
#include <cstring>
#include <vector>

int main()
{
    vncEncodeZRLE encoder;
    rfbPixelFormat format = winvnc_test_true_colour_16();
    encoder.SetLocalFormat(format, 8, 8);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "ZRLE 16-bit remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 8, 8), "ZRLE 16-bit local format should initialize");
    encoder.m_use_zywrle = FALSE;
    encoder.set_use_zstd(false);

    const std::uint16_t pixel = 0x5294;
    std::vector<BYTE> source(8 * 8 * sizeof(pixel));
    for (std::size_t i = 0; i < source.size(); i += sizeof(pixel)) {
        std::memcpy(source.data() + i, &pixel, sizeof(pixel));
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(8, 8));

    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), winvnc_test_rect(0, 0, 8, 8));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader, "ZRLE 16-bit encoder should emit compressed payload");
    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingZRLE, "unexpected ZRLE 16-bit encoding");
    const auto *zrle = reinterpret_cast<const rfbZRLEHeader *>(dest.data() + sz_rfbFramebufferUpdateRectHeader);
    winvnc_test_expect(winvnc_test_host32(zrle->length) == encoded - sz_rfbFramebufferUpdateRectHeader - sz_rfbZRLEHeader, "unexpected ZRLE 16-bit payload length");
    return 0;
}
