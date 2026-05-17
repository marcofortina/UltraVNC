// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableShutdown.h"

#include <atomic>
#include <csignal>

namespace uvnc {
namespace winvnc {
namespace portable {
namespace {

std::atomic<bool> g_shutdownRequested(false);

void handleShutdownSignal(int)
{
    g_shutdownRequested.store(true, std::memory_order_relaxed);
}

} // namespace

void ShutdownState::Clear()
{
    g_shutdownRequested.store(false, std::memory_order_relaxed);
}

void ShutdownState::Request()
{
    g_shutdownRequested.store(true, std::memory_order_relaxed);
}

bool ShutdownState::IsRequested()
{
    return g_shutdownRequested.load(std::memory_order_relaxed);
}

bool ShutdownState::InstallSignalHandlers()
{
#if defined(_WIN32) || defined(WIN32)
    return true;
#else
    return std::signal(SIGINT, handleShutdownSignal) != SIG_ERR &&
           std::signal(SIGTERM, handleShutdownSignal) != SIG_ERR;
#endif
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
