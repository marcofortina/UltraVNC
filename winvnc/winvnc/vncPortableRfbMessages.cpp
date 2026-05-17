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

} // namespace portable
} // namespace winvnc
} // namespace uvnc
