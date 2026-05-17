// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncEncodeTight.h"

int main()
{
    vncEncodeTight encoder;

    RECT large_rect = {0, 0, 2048, 2048};
    const UINT split_count = encoder.NumCodedRects(large_rect);
    winvnc_test_expect(split_count > 1, "large tight rect should be split without LastRect markers");

    encoder.EnableLastRect(TRUE);
    winvnc_test_expect(encoder.NumCodedRects(large_rect) == 0, "large tight rect should use LastRect marker termination when enabled");

    RECT small_rect = {0, 0, 16, 16};
    winvnc_test_expect(encoder.NumCodedRects(small_rect) == 1, "small tight rect should stay as one coded rect");

    return 0;
}
