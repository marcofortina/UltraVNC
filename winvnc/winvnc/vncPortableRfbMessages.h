// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
#define UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H

#include "rfb.h"

namespace uvnc {
namespace winvnc {
namespace portable {

struct FramebufferUpdateRequest {
    bool incremental;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

struct KeyEvent {
    bool down;
    CARD32 keysym;
};

struct PointerEvent {
    CARD8 buttonMask;
    unsigned int x;
    unsigned int y;
};

bool DecodeFramebufferUpdateRequest(const rfbFramebufferUpdateRequestMsg& message,
                                    FramebufferUpdateRequest& out);
rfbFramebufferUpdateRequestMsg EncodeFramebufferUpdateRequest(const FramebufferUpdateRequest& request);
bool DecodeKeyEvent(const rfbKeyEventMsg& message, KeyEvent& out);
rfbKeyEventMsg EncodeKeyEvent(const KeyEvent& event);
bool DecodePointerEvent(const rfbPointerEventMsg& message, PointerEvent& out);
rfbPointerEventMsg EncodePointerEvent(const PointerEvent& event);
bool DecodeSetPixelFormat(const rfbSetPixelFormatMsg& message, rfbPixelFormat& out);
rfbSetPixelFormatMsg EncodeSetPixelFormat(const rfbPixelFormat& format);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
