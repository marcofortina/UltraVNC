// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_INPUT_BACKEND_H
#define UVNC_WINVNC_LINUX_INPUT_BACKEND_H

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxinput {

enum class InputBackend {
    Auto,
    None,
    XTest,
};

bool ParseInputBackendName(const std::string& name, InputBackend& backend);
const char *InputBackendName(InputBackend backend);
const char *InputBackendDescription(InputBackend backend);

bool IsInputBackendRuntimeAvailable(InputBackend backend);
bool ResolveInputBackend(InputBackend requested,
                         InputBackend& resolved,
                         std::string *error = nullptr);

} // namespace linuxinput
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_INPUT_BACKEND_H
