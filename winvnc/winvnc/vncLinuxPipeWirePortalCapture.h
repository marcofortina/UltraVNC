// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H
#define UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H

#include <cstdint>
#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

enum class PipeWirePortalRuntimeState {
    Built,
    MissingBuildDependencies,
    MissingWaylandSession,
    MissingPortalBus,
};

struct PipeWirePortalSessionRequest {
    std::string sessionToken;
    std::string handleToken;
    bool requestCursorMetadata;

    PipeWirePortalSessionRequest();
};

struct PipeWirePortalSourceRequest {
    bool screens;
    bool windows;
    bool virtualMonitors;

    PipeWirePortalSourceRequest();
};

struct PipeWireStreamDescriptor {
    std::uint32_t nodeId;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int bytesPerPixel;

    PipeWireStreamDescriptor();
};

class PipeWirePortalCaptureBackend {
public:
    static bool BuildAvailable();
    static bool RuntimeAvailable(std::string *reason = nullptr);
    static bool CaptureImplemented();
    static PipeWirePortalRuntimeState RuntimeState(std::string *reason = nullptr);
    static const char *RuntimeStateName(PipeWirePortalRuntimeState state);
    static PipeWirePortalSessionRequest DefaultSessionRequest();
    static PipeWirePortalSourceRequest DefaultSourceRequest();
    static bool ValidateSessionRequest(const PipeWirePortalSessionRequest& request, std::string *error = nullptr);
    static PipeWireStreamDescriptor DefaultStreamDescriptor();
    static bool ValidateStreamDescriptor(const PipeWireStreamDescriptor& descriptor, std::string *error = nullptr);
};

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H
