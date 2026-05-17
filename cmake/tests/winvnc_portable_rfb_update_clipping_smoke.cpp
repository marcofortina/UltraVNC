// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbUpdate.h"

#include <cassert>
#include <cstring>
#include <vector>

using namespace uvnc::winvnc::portable;

int main()
{
    Framebuffer framebuffer(8, 6, ServerConfig::DefaultPixelFormat());
    framebuffer.Fill(0x40);

    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 6;
    request.y = 4;
    request.width = 8;
    request.height = 8;

    const std::vector<CARD8> updateBytes = RawFramebufferUpdateBytes(framebuffer, request);
    assert(updateBytes.size() == sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader + 2 * 2 * 4);

    rfbFramebufferUpdateMsg update;
    std::memcpy(&update, updateBytes.data(), sizeof(update));
    assert(Swap16IfLE(update.nRects) == 1);

    rfbFramebufferUpdateRectHeader rect;
    std::memcpy(&rect, updateBytes.data() + sz_rfbFramebufferUpdateMsg, sizeof(rect));
    assert(Swap16IfLE(rect.r.x) == 6);
    assert(Swap16IfLE(rect.r.y) == 4);
    assert(Swap16IfLE(rect.r.w) == 2);
    assert(Swap16IfLE(rect.r.h) == 2);
    return 0;
}
