// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeUltra2.h"

#include <cstring>
#include <vector>

int main()
{
    vncEncodeUltra2 encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 2, 2);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "ultra2 remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 2, 2), "ultra2 local format should initialize");

    const std::vector<BYTE> source = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c,
        0x0d, 0x0e, 0x0f, 0x10,
    };
    std::vector<BYTE> dest(encoder.RequiredBuffSize(2, 2));

    const UINT encoded = encoder.EncodeRect(const_cast<BYTE *>(source.data()), nullptr, dest.data(), winvnc_test_rect(0, 0, 2, 2));
    winvnc_test_expect(encoded == sz_rfbFramebufferUpdateRectHeader + source.size(), "small ultra2 rect should fall back to raw");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingRaw, "unexpected ultra2 fallback encoding");
    winvnc_test_expect(std::memcmp(dest.data() + sz_rfbFramebufferUpdateRectHeader, source.data(), source.size()) == 0, "ultra2 fallback payload changed unexpectedly");

    return 0;
}
