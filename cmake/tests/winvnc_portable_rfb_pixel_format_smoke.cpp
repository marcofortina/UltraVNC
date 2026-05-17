// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"
#include "vncPortableServerConfig.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    rfbPixelFormat format = ServerConfig::DefaultPixelFormat();
    format.bitsPerPixel = 16;
    format.depth = 16;
    format.redMax = 31;
    format.greenMax = 63;
    format.blueMax = 31;
    format.redShift = 11;
    format.greenShift = 5;
    format.blueShift = 0;

    const rfbSetPixelFormatMsg wire = EncodeSetPixelFormat(format);
    rfbPixelFormat decoded;
    assert(DecodeSetPixelFormat(wire, decoded));
    assert(decoded.bitsPerPixel == 16);
    assert(decoded.depth == 16);
    assert(decoded.redMax == 31);
    assert(decoded.greenMax == 63);
    assert(decoded.blueMax == 31);
    assert(decoded.redShift == 11);

    rfbSetPixelFormatMsg invalid = wire;
    invalid.type = rfbFramebufferUpdateRequest;
    assert(!DecodeSetPixelFormat(invalid, decoded));
    return 0;
}
