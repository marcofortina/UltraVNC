// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    std::string error;

    config.SetDesktopName("linux-memory-server");
    assert(config.Validate(&error));

    config.SetDesktopName("");
    assert(!config.Validate(&error));
    assert(error == "desktop name must not be empty");

    config.SetDesktopName(std::string(1025, 'x'));
    assert(!config.Validate(&error));
    assert(error == "desktop name is too long");
    return 0;
}
