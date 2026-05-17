// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncencoder.h"

int main()
{
    vncEncoder encoder;

    rfbPixelFormat local = winvnc_test_true_colour_32();
    rfbPixelFormat remote = winvnc_test_true_colour_32();

    local.bitsPerPixel = 24;
    winvnc_test_expect(!encoder.SetLocalFormat(local, 2, 2), "24-bit local format should be rejected");

    local = winvnc_test_true_colour_32();
    encoder.SetLocalFormat(local, 2, 2);

    remote.bitsPerPixel = 24;
    winvnc_test_expect(!encoder.SetRemoteFormat(remote), "24-bit remote format should be rejected");

    remote = winvnc_test_true_colour_32();
    remote.trueColour = 0;
    remote.bitsPerPixel = 16;
    winvnc_test_expect(!encoder.SetRemoteFormat(remote), "non-8-bit remote palette format should be rejected");

    remote = winvnc_test_true_colour_32();
    winvnc_test_expect(encoder.SetRemoteFormat(remote), "valid 32-bit remote format should be accepted");

    return 0;
}
