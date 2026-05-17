// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"

namespace uvnc {
namespace winvnc {
namespace portable {

bool DecodeFramebufferUpdateRequest(const rfbFramebufferUpdateRequestMsg& message,
                                    FramebufferUpdateRequest& out)
{
    if (message.type != rfbFramebufferUpdateRequest) {
        return false;
    }
    out.incremental = message.incremental != 0;
    out.x = Swap16IfLE(message.x);
    out.y = Swap16IfLE(message.y);
    out.width = Swap16IfLE(message.w);
    out.height = Swap16IfLE(message.h);
    return true;
}

rfbFramebufferUpdateRequestMsg EncodeFramebufferUpdateRequest(const FramebufferUpdateRequest& request)
{
    rfbFramebufferUpdateRequestMsg message;
    message.type = rfbFramebufferUpdateRequest;
    message.incremental = request.incremental ? 1 : 0;
    message.x = Swap16IfLE(static_cast<CARD16>(request.x));
    message.y = Swap16IfLE(static_cast<CARD16>(request.y));
    message.w = Swap16IfLE(static_cast<CARD16>(request.width));
    message.h = Swap16IfLE(static_cast<CARD16>(request.height));
    return message;
}

bool DecodeKeyEvent(const rfbKeyEventMsg& message, KeyEvent& out)
{
    if (message.type != rfbKeyEvent) {
        return false;
    }
    out.down = message.down != 0;
    out.keysym = Swap32IfLE(message.key);
    return true;
}

rfbKeyEventMsg EncodeKeyEvent(const KeyEvent& event)
{
    rfbKeyEventMsg message;
    message.type = rfbKeyEvent;
    message.down = event.down ? 1 : 0;
    message.pad = 0;
    message.key = Swap32IfLE(event.keysym);
    return message;
}

bool DecodePointerEvent(const rfbPointerEventMsg& message, PointerEvent& out)
{
    if (message.type != rfbPointerEvent) {
        return false;
    }
    out.buttonMask = message.buttonMask;
    out.x = Swap16IfLE(message.x);
    out.y = Swap16IfLE(message.y);
    return true;
}

rfbPointerEventMsg EncodePointerEvent(const PointerEvent& event)
{
    rfbPointerEventMsg message;
    message.type = rfbPointerEvent;
    message.buttonMask = event.buttonMask;
    message.x = Swap16IfLE(static_cast<CARD16>(event.x));
    message.y = Swap16IfLE(static_cast<CARD16>(event.y));
    return message;
}

bool DecodeSetPixelFormat(const rfbSetPixelFormatMsg& message, rfbPixelFormat& out)
{
    if (message.type != rfbSetPixelFormat) {
        return false;
    }
    out = message.format;
    out.redMax = Swap16IfLE(out.redMax);
    out.greenMax = Swap16IfLE(out.greenMax);
    out.blueMax = Swap16IfLE(out.blueMax);
    return true;
}

rfbSetPixelFormatMsg EncodeSetPixelFormat(const rfbPixelFormat& format)
{
    rfbSetPixelFormatMsg message;
    message.type = rfbSetPixelFormat;
    message.pad1 = 0;
    message.pad2 = 0;
    message.format = format;
    message.format.redMax = Swap16IfLE(message.format.redMax);
    message.format.greenMax = Swap16IfLE(message.format.greenMax);
    message.format.blueMax = Swap16IfLE(message.format.blueMax);
    return message;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
