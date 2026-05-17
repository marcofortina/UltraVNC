// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencoder.h"

#include <vector>

int main()
{
    vncEncoder encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();
    encoder.SetLocalFormat(format, 4, 4);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "remote format should initialize encoder translation");
    encoder.SetBufferOffset(1, 2);

    std::vector<BYTE> source(4 * 4 * 4, 0x7f);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(2, 2));

    const rfb::Rect rect = winvnc_test_rect(1, 2, 3, 4);
    const UINT encoded = encoder.EncodeRect(source.data(), dest.data(), rect);
    winvnc_test_expect(encoded == sz_rfbFramebufferUpdateRectHeader + 16, "unexpected offset encoded size");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.x) == 0, "offset x should be relative to monitor offset");
    winvnc_test_expect(winvnc_test_host16(header->r.y) == 0, "offset y should be relative to monitor offset");
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 2, "unexpected offset width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 2, "unexpected offset height");

    return 0;
}
