// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableShutdown.h"

int main()
{
    using uvnc::winvnc::portable::ShutdownState;

    ShutdownState::Clear();
    winvnc_test_expect(!ShutdownState::IsRequested(), "shutdown should start cleared");

    ShutdownState::Request();
    winvnc_test_expect(ShutdownState::IsRequested(), "shutdown request was not recorded");

    ShutdownState::Clear();
    winvnc_test_expect(!ShutdownState::IsRequested(), "shutdown clear failed");
    winvnc_test_expect(ShutdownState::InstallSignalHandlers(), "signal handler installation failed");

    return 0;
}
