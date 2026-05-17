// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC Projects. All Rights Reserved.
//

#include "common/stdhdrs.h"
#include "common/UltraVncZ.h"

namespace {

bool testZlibCompresses()
{
    UltraVncZ compressor;
    compressor.set_use_zstd(false);

    BYTE input[256] = {};
    BYTE output[1024] = {};
    for (UINT i = 0; i < sizeof(input); ++i) {
        input[i] = static_cast<BYTE>(i & 0x0f);
    }

    const UINT written = compressor.compress(6, sizeof(input), sizeof(output), input, output);
    return written > 0 && written < sizeof(output);
}

bool testZstdCompresses()
{
    UltraVncZ compressor;
    compressor.set_use_zstd(true);

    BYTE input[256] = {};
    BYTE output[1024] = {};
    for (UINT i = 0; i < sizeof(input); ++i) {
        input[i] = static_cast<BYTE>((i * 3) & 0xff);
    }

    const UINT written = compressor.compress(9, sizeof(input), sizeof(output), input, output);
    return written > 0 && written < sizeof(output);
}

}

int main()
{
    if (!testZlibCompresses()) {
        return 1;
    }
    if (!testZstdCompresses()) {
        return 1;
    }
    return 0;
}
