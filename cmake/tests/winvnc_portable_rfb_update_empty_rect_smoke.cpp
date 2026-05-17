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
#include "vncPortableServerConfig.h"

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
    request.x = 9;
    request.y = 9;
    request.width = 2;
    request.height = 2;

    const std::vector<CARD8> updateBytes = RawFramebufferUpdateBytes(framebuffer, request);
    assert(updateBytes.size() == sz_rfbFramebufferUpdateMsg);

    rfbFramebufferUpdateMsg update;
    std::memcpy(&update, updateBytes.data(), sizeof(update));
    assert(update.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(update.nRects) == 0);
    return 0;
}
