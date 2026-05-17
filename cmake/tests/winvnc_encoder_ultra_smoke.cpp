// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeUltra.h"

#include <cstring>
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
    vncEncodeUltra encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 2, 2);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "ultra remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 2, 2), "ultra local format should initialize");

    const std::vector<BYTE> source = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c,
        0x0d, 0x0e, 0x0f, 0x10,
    };
    std::vector<BYTE> dest(encoder.RequiredBuffSize(2, 2));
    CountingSocket socket;

    const UINT encoded = encoder.EncodeRect(const_cast<BYTE *>(source.data()), &socket, dest.data(), winvnc_test_rect(0, 0, 2, 2));
    winvnc_test_expect(encoded == sz_rfbFramebufferUpdateRectHeader + source.size(), "small ultra rect should fall back to raw");

    const auto *header = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(dest.data());
    winvnc_test_expect(winvnc_test_host32(header->encoding) == rfbEncodingRaw, "unexpected ultra fallback encoding");
    winvnc_test_expect(std::memcmp(dest.data() + sz_rfbFramebufferUpdateRectHeader, source.data(), source.size()) == 0, "ultra fallback payload changed unexpectedly");
    winvnc_test_expect(socket.sends == 0, "raw ultra fallback should not queue partial sends");

    return 0;
}
