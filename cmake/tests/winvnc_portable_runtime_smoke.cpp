// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableRuntime.h"

#include <cstdint>

int main()
{
    using uvnc::winvnc::portable::Runtime;

    const std::uint64_t before = Runtime::MonotonicMilliseconds();
    Runtime::SleepMilliseconds(1);
    const std::uint64_t after = Runtime::MonotonicMilliseconds();

    winvnc_test_expect(after >= before, "monotonic time moved backwards");
    winvnc_test_expect(Runtime::CurrentProcessId() != 0, "process id was not populated");
    winvnc_test_expect(Runtime::GetEnvironment(nullptr, "fallback") == "fallback", "null environment fallback failed");
    winvnc_test_expect(!Runtime::HasEnvironment(nullptr), "null environment name should not exist");

    return 0;
}
