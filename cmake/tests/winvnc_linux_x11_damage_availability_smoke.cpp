// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"

#include <iostream>

using namespace uvnc::winvnc::linuxfb;

int main()
{
    std::cout << "x11-xdamage-build-available=" << (X11DesktopSource::IsXDamageBuildAvailable() ? "yes" : "no") << "\n";
    std::cout << "x11-xdamage-runtime-available=" << (X11DesktopSource::IsXDamageRuntimeAvailable() ? "yes" : "no") << "\n";
    return 0;
}
