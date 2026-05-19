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

    config.SetUpdatePacingMs(33);
    assert(config.Validate(&error));
    assert(config.UpdatePacingMs() == 33);

    config.SetUpdatePacingMs(5001);
    assert(!config.Validate(&error));
    assert(error.find("update pacing") != std::string::npos);
    return 0;
}
