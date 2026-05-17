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
    assert(config.FillByte() == 0x22);
    config.SetFillByte(0xab);
    assert(config.FillByte() == 0xab);
    std::string error;
    assert(config.Validate(&error));
    assert(error.empty());
    return 0;
}
