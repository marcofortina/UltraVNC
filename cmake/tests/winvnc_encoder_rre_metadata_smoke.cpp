// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencoderre.h"

int main()
{
    vncEncodeRRE encoder;
    rfbPixelFormat format = winvnc_test_true_colour_32();
    encoder.SetLocalFormat(format, 8, 8);
    winvnc_test_expect(encoder.SetRemoteFormat(format), "remote format should initialize RRE metadata test");

    const rfb::Rect rect = winvnc_test_rect(0, 0, 8, 8);
    winvnc_test_expect(encoder.NumCodedRects(rect) == 1, "RRE encoder should report one coded rect");
    winvnc_test_expect(encoder.RequiredBuffSize(8, 8) == sz_rfbFramebufferUpdateRectHeader + (8 * 8 * 4), "unexpected RRE required buffer size");

    return 0;
}
