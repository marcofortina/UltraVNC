// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxPipeWirePortalCapture.h"

#include <cstdlib>
#include <cstring>

namespace uvnc {
namespace winvnc {
namespace linuxfb {
namespace {

void SetReason(std::string *reason, const char *message)
{
    if (reason) {
        *reason = message;
    }
}

bool HasEnv(const char *name)
{
    const char *value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
}

bool IsWaylandSession()
{
    const char *sessionType = std::getenv("XDG_SESSION_TYPE");
    return (sessionType && std::strcmp(sessionType, "wayland") == 0) || HasEnv("WAYLAND_DISPLAY");
}

} // namespace

bool PipeWirePortalCaptureBackend::BuildAvailable()
{
#ifdef UVNC_HAVE_PIPEWIRE_PORTAL
    return true;
#else
    return false;
#endif
}

PipeWirePortalRuntimeState PipeWirePortalCaptureBackend::RuntimeState(std::string *reason)
{
    if (!BuildAvailable()) {
        SetReason(reason, "PipeWire/XDG portal capture support was not built");
        return PipeWirePortalRuntimeState::MissingBuildDependencies;
    }
    if (!IsWaylandSession()) {
        SetReason(reason, "Wayland session is not available");
        return PipeWirePortalRuntimeState::MissingWaylandSession;
    }
    if (!HasEnv("DBUS_SESSION_BUS_ADDRESS")) {
        SetReason(reason, "D-Bus session bus is not available");
        return PipeWirePortalRuntimeState::MissingPortalBus;
    }

    SetReason(reason, "PipeWire/XDG portal runtime appears available");
    return PipeWirePortalRuntimeState::Built;
}

bool PipeWirePortalCaptureBackend::RuntimeAvailable(std::string *reason)
{
    return RuntimeState(reason) == PipeWirePortalRuntimeState::Built;
}

const char *PipeWirePortalCaptureBackend::RuntimeStateName(PipeWirePortalRuntimeState state)
{
    switch (state) {
    case PipeWirePortalRuntimeState::Built:
        return "available";
    case PipeWirePortalRuntimeState::MissingBuildDependencies:
        return "missing-build-dependencies";
    case PipeWirePortalRuntimeState::MissingWaylandSession:
        return "missing-wayland-session";
    case PipeWirePortalRuntimeState::MissingPortalBus:
        return "missing-portal-bus";
    }
    return "unknown";
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
