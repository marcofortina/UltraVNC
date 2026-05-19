// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11CursorSource.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace uvnc::winvnc::linuxfb;

int main()
{
    std::string reason;
    const bool build = X11CursorSource::IsBuildAvailable();
    const bool runtime = X11CursorSource::IsRuntimeAvailable(std::string(), &reason);
    std::cout << "x11-cursor-build-available=" << (build ? "yes" : "no") << "\n";
    std::cout << "x11-cursor-runtime-available=" << (runtime ? "yes" : "no") << "\n";
    std::cout << "x11-cursor-unavailable-reason=" << (runtime ? "" : reason) << "\n";
    if (runtime) {
        uvnc::winvnc::portable::CursorShape shape;
        X11CursorSource source;
        assert(source.GetCursorShape(shape, &reason));
        assert(shape.Valid());
    }
    return 0;
}
