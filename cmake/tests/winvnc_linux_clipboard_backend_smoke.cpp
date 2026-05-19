// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxClipboardBackend.h"

#include <cassert>
#include <iostream>

using namespace uvnc::winvnc::linuxclipboard;

int main()
{
    std::string reason;
    const bool available = X11ClipboardBackend::RuntimeAvailable(&reason);
    std::cout << "x11-clipboard-runtime=" << (available ? "available" : "unavailable") << "\n";
    if (!available) {
        std::cout << "x11-clipboard-reason=" << reason << "\n";
    }
    return 0;
}
