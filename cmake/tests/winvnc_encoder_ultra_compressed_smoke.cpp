// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeUltra.h"

#include <vector>

int main()
{
    vncEncodeUltra encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 128, 128);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "ultra compressed remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 128, 128), "ultra compressed local format should initialize");

    std::vector<BYTE> source(128 * 128 * 4, 0x55);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(128, 128));
    winvnc_test_counting_socket socket;

    const UINT encoded = encoder.EncodeRect(source.data(), &socket, dest.data(), winvnc_test_rect(0, 0, 128, 128));
    winvnc_test_expect(encoded > sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader, "ultra compressed rect should contain payload");
    winvnc_test_expect(encoded < sz_rfbFramebufferUpdateRectHeader + source.size(), "ultra compressed rect should be smaller than raw");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host16(header->r.w) == 128, "unexpected ultra compressed width");
    winvnc_test_expect(winvnc_test_host16(header->r.h) == 128, "unexpected ultra compressed height");
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingUltra, "unexpected ultra compressed encoding");
    winvnc_test_expect(socket.queued_sends == 0, "single ultra compressed rect should not queue partial sends");

    const auto *ultra_header = reinterpret_cast<const rfbZlibHeader *>(dest.data() + sz_rfbFramebufferUpdateRectHeader);
    winvnc_test_expect(winvnc_test_host32(ultra_header->nBytes) == encoded - sz_rfbFramebufferUpdateRectHeader - sz_rfbZlibHeader, "unexpected ultra compressed payload size");

    return 0;
}
