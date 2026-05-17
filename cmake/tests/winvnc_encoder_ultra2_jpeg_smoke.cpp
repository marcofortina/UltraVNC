// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeUltra2.h"

#include <vector>

int main()
{
    vncEncodeUltra2 encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 16, 16);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "ultra2 jpeg remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 16, 16), "ultra2 jpeg local format should initialize");
    encoder.SetQualityLevel(6);

    std::vector<BYTE> source(16 * 16 * 4);
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            const size_t offset = static_cast<size_t>((y * 16 + x) * 4);
            source[offset + 0] = static_cast<BYTE>(x * 16);
            source[offset + 1] = static_cast<BYTE>(y * 16);
            source[offset + 2] = static_cast<BYTE>((x + y) * 8);
            source[offset + 3] = 0xff;
        }
    }
    std::vector<BYTE> dest(encoder.RequiredBuffSize(16, 16));

    const UINT encoded = encoder.EncodeRect(source.data(), nullptr, dest.data(), winvnc_test_rect(0, 0, 16, 16));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader, "ultra2 jpeg rect should contain payload");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 16, "unexpected ultra2 jpeg width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 16, "unexpected ultra2 jpeg height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingUltra2, "unexpected ultra2 jpeg encoding");

    const auto *ultra2_header = reinterpret_cast<const rfbZlibHeader *>(dest.data() + sz_rfbFramebufferUpdateRectHeader);
    winvnc_test_expect(winvnc_test_host32(ultra2_header->nBytes) == encoded - sz_rfbFramebufferUpdateRectHeader - sz_rfbZlibHeader, "unexpected ultra2 jpeg payload size");

    return 0;
}
