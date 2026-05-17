// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeXZ.h"

#include <vector>

int main()
{
    vncEncodeXZ encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 8, 8);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "xz remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 8, 8), "xz local format should initialize");
    encoder.SetCompressLevel(1);

    std::vector<BYTE> source(8 * 8 * 4, 0x11);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(8, 8));
    rfb::RectVector rects;
    rects.push_back(winvnc_test_rect(0, 0, 8, 8));
    winvnc_test_counting_socket socket;

    const UINT encoded = encoder.EncodeBulkRects(rects, source.data(), dest.data(), &socket);
    winvnc_test_expect(encoded == TRUE, "xz bulk encoding should report success");
    winvnc_test_expect(socket.exact_sends == 2, "xz encoder should send header and payload");
    winvnc_test_expect(socket.exact_bytes > sz_rfbFramebufferUpdateRectHeader, "xz socket output should include compressed payload");

    return 0;
}
