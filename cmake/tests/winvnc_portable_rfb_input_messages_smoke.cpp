// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    KeyEvent key;
    key.down = true;
    key.keysym = 0xff0d;
    const rfbKeyEventMsg keyWire = EncodeKeyEvent(key);
    KeyEvent decodedKey;
    assert(DecodeKeyEvent(keyWire, decodedKey));
    assert(decodedKey.down);
    assert(decodedKey.keysym == 0xff0d);

    rfbKeyEventMsg invalidKey = keyWire;
    invalidKey.type = rfbPointerEvent;
    assert(!DecodeKeyEvent(invalidKey, decodedKey));

    PointerEvent pointer;
    pointer.buttonMask = 1;
    pointer.x = 123;
    pointer.y = 456;
    const rfbPointerEventMsg pointerWire = EncodePointerEvent(pointer);
    PointerEvent decodedPointer;
    assert(DecodePointerEvent(pointerWire, decodedPointer));
    assert(decodedPointer.buttonMask == 1);
    assert(decodedPointer.x == 123);
    assert(decodedPointer.y == 456);

    rfbPointerEventMsg invalidPointer = pointerWire;
    invalidPointer.type = rfbKeyEvent;
    assert(!DecodePointerEvent(invalidPointer, decodedPointer));

    return 0;
}
