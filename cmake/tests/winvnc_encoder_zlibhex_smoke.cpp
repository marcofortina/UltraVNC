// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeZlibHex.h"

#include <vector>

int main()
{
    vncEncodeZlibHex encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 16, 16);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "zlibhex remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 16, 16), "zlibhex local format should initialize");

    std::vector<BYTE> source(16 * 16 * 4, 0x42);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(16, 16));
    winvnc_test_counting_socket socket;

    RECT rect = {};
    rect.right = 16;
    rect.bottom = 16;

    const UINT encoded = encoder.EncodeRect(source.data(), &socket, dest.data(), rect);
    winvnc_test_expect(encoded > 0, "zlibhex should encode the tile");
    winvnc_test_expect(socket.queued_sends > 0, "zlibhex should queue encoded data to the socket");
    winvnc_test_expect(socket.queued_bytes > 0, "zlibhex socket output should not be empty");

    return 0;
}
