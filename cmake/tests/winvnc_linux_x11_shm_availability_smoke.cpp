// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"

#include <cassert>

using namespace uvnc::winvnc::linuxfb;

int main()
{
    const bool buildAvailable = X11DesktopSource::IsXShmBuildAvailable();
    const bool runtimeAvailable = X11DesktopSource::IsXShmRuntimeAvailable();

    if (!buildAvailable) {
        assert(!runtimeAvailable);
    }
    return 0;
}
