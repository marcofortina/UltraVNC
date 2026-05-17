// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#pragma once

#include "rfb.h"
#include "rfbRect.h"
#include "vsocket_portable.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

inline rfbPixelFormat winvnc_test_true_colour_32()
{
    rfbPixelFormat format = {};
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.bigEndian = 0;
    format.trueColour = 1;
    format.redMax = 255;
    format.greenMax = 255;
    format.blueMax = 255;
    format.redShift = 16;
    format.greenShift = 8;
    format.blueShift = 0;
    return format;
}

inline rfbPixelFormat winvnc_test_true_colour_16()
{
    rfbPixelFormat format = {};
    format.bitsPerPixel = 16;
    format.depth = 16;
    format.bigEndian = 0;
    format.trueColour = 1;
    format.redMax = 31;
    format.greenMax = 63;
    format.blueMax = 31;
    format.redShift = 11;
    format.greenShift = 5;
    format.blueShift = 0;
    return format;
}

inline rfb::Rect winvnc_test_rect(int left, int top, int right, int bottom)
{
    rfb::Rect rect;
    rect.tl.x = left;
    rect.tl.y = top;
    rect.br.x = right;
    rect.br.y = bottom;
    return rect;
}

inline void winvnc_test_expect(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

inline std::uint16_t winvnc_test_host16(CARD16 value)
{
    return Swap16IfLE(value);
}

inline std::uint32_t winvnc_test_host32(CARD32 value)
{
    return Swap32IfLE(value);
}

class winvnc_test_counting_socket : public VSocket {
public:
    void SendExactQueue(char *, int length) override
    {
        queued_bytes += length;
        queued_sends += 1;
    }

    bool SendExact(const char *, int length) override
    {
        exact_bytes += length;
        exact_sends += 1;
        return exact_result;
    }

    int queued_sends = 0;
    int queued_bytes = 0;
    int exact_sends = 0;
    int exact_bytes = 0;
    bool exact_result = true;
};
