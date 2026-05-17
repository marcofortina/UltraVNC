// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#include "translate.h"

#include <cstdlib>

namespace {

rfbPixelFormat rgb332Format()
{
    rfbPixelFormat format = {};
    format.bitsPerPixel = 8;
    format.depth = 8;
    format.trueColour = 1;
    format.redMax = 7;
    format.greenMax = 7;
    format.blueMax = 3;
    format.redShift = 0;
    format.greenShift = 3;
    format.blueShift = 6;
    return format;
}

rfbPixelFormat rgb565Format()
{
    rfbPixelFormat format = {};
    format.bitsPerPixel = 16;
    format.depth = 16;
    format.trueColour = 1;
    format.redMax = 31;
    format.greenMax = 63;
    format.blueMax = 31;
    format.redShift = 11;
    format.greenShift = 5;
    format.blueShift = 0;
    return format;
}

bool testTranslateNoneCopiesRows()
{
    rfbPixelFormat format = rgb332Format();
    const char source[] = {1, 2, 3, 4, 5, 6};
    char dest[] = {0, 0, 0, 0};

    rfbTranslateNone(nullptr, &format, &format,
        const_cast<char *>(source), dest, 3, 2, 2);

    return dest[0] == 1 && dest[1] == 2 && dest[2] == 4 && dest[3] == 5;
}

bool testSingleTableTranslationInitializes()
{
    rfbPixelFormat in = rgb332Format();
    rfbPixelFormat out = rgb565Format();
    char *table = nullptr;

    rfbInitTrueColourSingleTableFns[1](&table, &in, &out);
    if (table == nullptr) {
        return false;
    }

    const unsigned char source[] = {0, 0xff};
    unsigned short dest[] = {0, 0};
    rfbTranslateWithSingleTableFns[0][1](table, &in, &out,
        reinterpret_cast<char *>(const_cast<unsigned char *>(source)),
        reinterpret_cast<char *>(dest), 2, 2, 1);

    std::free(table);
    return dest[0] == 0 && dest[1] != 0;
}

}

int main()
{
    if (!testTranslateNoneCopiesRows()) {
        return 1;
    }
    if (!testSingleTableTranslationInitializes()) {
        return 1;
    }
    return 0;
}
