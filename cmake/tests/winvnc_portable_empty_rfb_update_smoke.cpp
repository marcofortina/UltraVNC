// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbUpdate.h"

#include <cassert>
#include <cstring>
#include <vector>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::vector<CARD8> bytes = EmptyFramebufferUpdateBytes();
    assert(bytes.size() == sz_rfbFramebufferUpdateMsg);

    rfbFramebufferUpdateMsg update;
    std::memcpy(&update, bytes.data(), sizeof(update));
    assert(update.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(update.nRects) == 0);
    return 0;
}
