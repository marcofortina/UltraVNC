// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencodecorre.h"

int main()
{
    vncEncodeCoRRE encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();

    encoder.SetLocalFormat(format, 64, 64);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "CoRRE split remote format should initialize");
    winvnc_test_expect(encoder.SetLocalFormat(format, 64, 64), "CoRRE split local format should initialize");
    winvnc_test_expect(encoder.NumCodedRects(winvnc_test_rect(0, 0, 64, 64)) > 1, "CoRRE large rect should split into several coded rects");

    return 0;
}
