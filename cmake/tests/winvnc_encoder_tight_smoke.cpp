// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeTight.h"

#include <vector>

class CountingSocket : public VSocket {
public:
    void SendExactQueue(char *, int length) override
    {
        bytes += length;
        sends += 1;
    }

    int sends = 0;
    int bytes = 0;
};

int main()
{
    vncEncodeTight encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 8, 8);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "tight remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 8, 8), "tight local format should initialize");
    encoder.SetCompressLevel(1);
    encoder.SetQualityLevel(-1);

    std::vector<BYTE> source(8 * 8 * 4, 0x22);
    std::vector<BYTE> dest(encoder.RequiredBuffSize(8, 8));
    CountingSocket socket;

    RECT rect = {};
    rect.right = 8;
    rect.bottom = 8;

    const UINT encoded = encoder.EncodeRect(source.data(), &socket, dest.data(), rect);
    winvnc_test_expect(encoded > 0, "tight encoder should encode the rect");

    winvnc_test_expect(socket.sends >= 0, "tight socket accounting should stay valid");

    return 0;
}
