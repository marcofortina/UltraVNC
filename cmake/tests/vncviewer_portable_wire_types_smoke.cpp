// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "rfb.h"

#include <cassert>

int main()
{
    assert(sizeof(CARD8) == 1);
    assert(sizeof(CARD16) == 2);
    assert(sizeof(CARD32) == 4);
    assert(sizeof(rfbFramebufferUpdateMsg) == sz_rfbFramebufferUpdateMsg);
    assert(sizeof(rfbFramebufferUpdateRequestMsg) >= sz_rfbFramebufferUpdateRequestMsg);
    return 0;
}
