// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxXTestInput.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxinput;

int main()
{
    const bool buildAvailable = XTestInputBackend::IsBuildAvailable();
    const bool runtimeAvailable = XTestInputBackend::IsAvailable();
    assert(!runtimeAvailable || buildAvailable);
    assert(std::string(XTestInputBackend::UnavailableReason()).find("XTest") != std::string::npos);

    std::string error;
    XTestInputBackend backend;
    if (!runtimeAvailable) {
        assert(!backend.InjectKeySym(0xff0d, true, &error));
        assert(!error.empty());
    }
    return 0;
}
