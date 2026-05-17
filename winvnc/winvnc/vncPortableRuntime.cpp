// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRuntime.h"

#include <chrono>
#include <cstdlib>
#include <thread>

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace uvnc {
namespace winvnc {
namespace portable {

std::uint64_t Runtime::MonotonicMilliseconds()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

void Runtime::SleepMilliseconds(unsigned int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

unsigned int Runtime::CurrentProcessId()
{
#if defined(_WIN32) || defined(WIN32)
    return static_cast<unsigned int>(GetCurrentProcessId());
#else
    return static_cast<unsigned int>(getpid());
#endif
}

bool Runtime::HasEnvironment(const char *name)
{
    return name != nullptr && std::getenv(name) != nullptr;
}

std::string Runtime::GetEnvironment(const char *name, const char *fallback)
{
    if (name == nullptr) {
        return fallback == nullptr ? std::string() : std::string(fallback);
    }

    const char *value = std::getenv(name);
    if (value == nullptr) {
        return fallback == nullptr ? std::string() : std::string(fallback);
    }

    return std::string(value);
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
