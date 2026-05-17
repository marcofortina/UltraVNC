// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbClientState.h"

#include <cassert>
#include <vector>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    RfbClientState state(config);

    assert(state.PixelFormat().bitsPerPixel == config.PixelFormat().bitsPerPixel);
    assert(state.Encodings().size() == 1);
    assert(state.Encodings()[0] == rfbEncodingRaw);

    std::vector<CARD32> encodings;
    encodings.push_back(rfbEncodingRaw);
    encodings.push_back(rfbEncodingTight);
    state.SetEncodings(encodings);
    assert(state.Encodings().size() == 2);
    assert(state.Encodings()[1] == rfbEncodingTight);

    KeyEvent key;
    key.down = true;
    key.keysym = 0xff0d;
    state.RecordKeyEvent(key);
    assert(state.KeyEventCount() == 1);
    assert(state.LastKeyEvent().down);
    assert(state.LastKeyEvent().keysym == 0xff0d);

    PointerEvent pointer;
    pointer.buttonMask = 1;
    pointer.x = 10;
    pointer.y = 20;
    state.RecordPointerEvent(pointer);
    assert(state.PointerEventCount() == 1);
    assert(state.LastPointerEvent().buttonMask == 1);
    assert(state.LastPointerEvent().x == 10);
    assert(state.LastPointerEvent().y == 20);

    state.RecordClientCutText(5);
    state.RecordClientCutText(7);
    assert(state.ClientCutTextMessages() == 2);
    assert(state.ClientCutTextBytes() == 12);
    return 0;
}
