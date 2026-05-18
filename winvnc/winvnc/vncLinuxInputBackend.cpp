// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxInputBackend.h"

#include "vncLinuxXTestInput.h"

namespace uvnc {
namespace winvnc {
namespace linuxinput {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

} // namespace

bool ParseInputBackendName(const std::string& name, InputBackend& backend)
{
    if (name == "auto") {
        backend = InputBackend::Auto;
        return true;
    }
    if (name == "none") {
        backend = InputBackend::None;
        return true;
    }
    if (name == "xtest") {
        backend = InputBackend::XTest;
        return true;
    }
    return false;
}

const char *InputBackendName(InputBackend backend)
{
    switch (backend) {
    case InputBackend::Auto:
        return "auto";
    case InputBackend::None:
        return "none";
    case InputBackend::XTest:
        return "xtest";
    }
    return "unknown";
}

const char *InputBackendDescription(InputBackend backend)
{
    switch (backend) {
    case InputBackend::Auto:
        return "auto-select XTest when available, otherwise disable Linux input injection";
    case InputBackend::None:
        return "disable Linux input injection";
    case InputBackend::XTest:
        return "X11 XTest input injection backend";
    }
    return "unknown input backend";
}

bool IsInputBackendRuntimeAvailable(InputBackend backend)
{
    switch (backend) {
    case InputBackend::Auto:
    case InputBackend::None:
        return true;
    case InputBackend::XTest:
        return XTestInputBackend::IsAvailable();
    }
    return false;
}

bool ResolveInputBackend(InputBackend requested,
                         InputBackend& resolved,
                         std::string *error)
{
    if (requested == InputBackend::Auto) {
        resolved = XTestInputBackend::IsAvailable() ? InputBackend::XTest : InputBackend::None;
        if (error) {
            error->clear();
        }
        return true;
    }

    if (!IsInputBackendRuntimeAvailable(requested)) {
        SetError(error, std::string("input backend is not available: ") + InputBackendName(requested));
        return false;
    }

    resolved = requested;
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace linuxinput
} // namespace winvnc
} // namespace uvnc
