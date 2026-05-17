// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_SHUTDOWN_H
#define UVNC_WINVNC_PORTABLE_SHUTDOWN_H

namespace uvnc {
namespace winvnc {
namespace portable {

class ShutdownState {
public:
    static void Clear();
    static void Request();
    static bool IsRequested();
    static bool InstallSignalHandlers();
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_SHUTDOWN_H
