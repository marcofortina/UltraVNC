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


PipeWirePortalSessionRequest::PipeWirePortalSessionRequest()
    : sessionToken("uvnc_session"),
      handleToken("uvnc_handle"),
      requestCursorMetadata(true)
{
}

PipeWirePortalSourceRequest::PipeWirePortalSourceRequest()
    : screens(true),
      windows(false),
      virtualMonitors(false)
{
}

PipeWireStreamDescriptor::PipeWireStreamDescriptor()
    : nodeId(0),
      width(0),
      height(0),
      stride(0),
      bytesPerPixel(4)
{
}

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

bool PipeWirePortalCaptureBackend::CaptureImplemented()
{
    return false;
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

PipeWirePortalSessionRequest PipeWirePortalCaptureBackend::DefaultSessionRequest()
{
    return PipeWirePortalSessionRequest();
}

PipeWirePortalSourceRequest PipeWirePortalCaptureBackend::DefaultSourceRequest()
{
    return PipeWirePortalSourceRequest();
}

bool PipeWirePortalCaptureBackend::ValidateSessionRequest(const PipeWirePortalSessionRequest& request, std::string *error)
{
    if (request.sessionToken.empty()) {
        SetReason(error, "PipeWire/XDG portal session token must not be empty");
        return false;
    }
    if (request.handleToken.empty()) {
        SetReason(error, "PipeWire/XDG portal handle token must not be empty");
        return false;
    }
    if (request.sessionToken.size() > 128 || request.handleToken.size() > 128) {
        SetReason(error, "PipeWire/XDG portal tokens are too long");
        return false;
    }
    SetReason(error, "");
    return true;
}

PipeWireStreamDescriptor PipeWirePortalCaptureBackend::DefaultStreamDescriptor()
{
    return PipeWireStreamDescriptor();
}

bool PipeWirePortalCaptureBackend::ValidateStreamDescriptor(const PipeWireStreamDescriptor& descriptor, std::string *error)
{
    if (descriptor.nodeId == 0) {
        SetReason(error, "PipeWire stream node id must be non-zero");
        return false;
    }
    if (descriptor.width == 0 || descriptor.height == 0) {
        SetReason(error, "PipeWire stream dimensions must be non-zero");
        return false;
    }
    if (descriptor.bytesPerPixel == 0) {
        SetReason(error, "PipeWire stream bytes per pixel must be non-zero");
        return false;
    }
    if (descriptor.stride < descriptor.width * descriptor.bytesPerPixel) {
        SetReason(error, "PipeWire stream stride is smaller than one row");
        return false;
    }
    SetReason(error, "");
    return true;
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
