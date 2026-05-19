// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_CURSOR_H
#define UVNC_WINVNC_PORTABLE_CURSOR_H

#include "rfb.h"

#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

struct CursorShape {
    unsigned int width;
    unsigned int height;
    unsigned int hotspotX;
    unsigned int hotspotY;
    std::vector<CARD8> bgra;

    CursorShape();
    bool Valid() const;
};

CursorShape DefaultArrowCursorShape();
CursorShape EmptyCursorShape();

std::vector<CARD8> EncodeRichCursorShapeUpdate(const CursorShape& shape);
std::vector<CARD8> EncodeXCursorShapeUpdate(const CursorShape& shape);
std::vector<CARD8> EncodeEmptyCursorShapeUpdate(CARD32 encoding);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_CURSOR_H