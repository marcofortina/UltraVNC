// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeZlib.h"

#include <vector>

int main()
{
    vncEncodeZlib encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 64, 64);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "zlib remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 64, 64), "zlib local format should initialize");
    encoder.SetCompressLevel(6);

    std::vector<BYTE> source(64 * 64 * 4, 0x33);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(64, 64));

    const UINT encoded = encoder.EncodeRect(source.data(), nullptr, dest.data(), winvnc_test_rect(0, 0, 64, 64), false);
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader, "zlib compressed rect should contain payload");
    winvnc_test_expect(encoded < sz_rfbFramebufferUpdateRectHeader + source.size(), "zlib compressed rect should be smaller than raw");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 64, "unexpected zlib compressed width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 64, "unexpected zlib compressed height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingZlib, "unexpected zlib compressed encoding");

    const auto *zlib_header = reinterpret_cast<const rfbZlibHeader *>(dest.data() + sz_rfbFramebufferUpdateRectHeader);
    winvnc_test_expect(winvnc_test_host32(zlib_header->nBytes) == encoded - sz_rfbFramebufferUpdateRectHeader - sz_rfbZlibHeader, "unexpected zlib compressed payload size");

    return 0;
}
