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

    config.SetBindAddress("127.0.0.1");
    assert(config.Validate(&error));

    config.SetBindAddress("0.0.0.0");
    assert(config.Validate(&error));

    config.SetBindAddress("localhost");
    assert(!config.Validate(&error));
    assert(error == "bind address must be a valid IPv4 address");

    config.SetBindAddress("999.1.1.1");
    assert(!config.Validate(&error));
    return 0;
}
