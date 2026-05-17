// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"

#include <cassert>

using uvnc::winvnc::portable::DecodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::EncodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::FramebufferUpdateRequest;

int main()
{
    FramebufferUpdateRequest request;
    request.incremental = true;
    request.x = 3;
    request.y = 5;
    request.width = 320;
    request.height = 200;

    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    FramebufferUpdateRequest decoded;
    assert(DecodeFramebufferUpdateRequest(wire, decoded));
    assert(decoded.incremental);
    assert(decoded.x == 3);
    assert(decoded.y == 5);
    assert(decoded.width == 320);
    assert(decoded.height == 200);

    rfbFramebufferUpdateRequestMsg invalid = wire;
    invalid.type = rfbKeyEvent;
    assert(!DecodeFramebufferUpdateRequest(invalid, decoded));

    return 0;
}
