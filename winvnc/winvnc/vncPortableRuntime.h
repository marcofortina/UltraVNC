// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RUNTIME_H
#define UVNC_WINVNC_PORTABLE_RUNTIME_H

#include <cstdint>
#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class Runtime {
public:
    static std::uint64_t MonotonicMilliseconds();
    static void SleepMilliseconds(unsigned int milliseconds);
    static unsigned int CurrentProcessId();
    static bool HasEnvironment(const char *name);
    static std::string GetEnvironment(const char *name, const char *fallback = "");
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RUNTIME_H
