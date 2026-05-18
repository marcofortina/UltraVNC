// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H
#define UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

enum class PipeWirePortalRuntimeState {
    Built,
    MissingBuildDependencies,
    MissingWaylandSession,
    MissingPortalBus,
};

class PipeWirePortalCaptureBackend {
public:
    static bool BuildAvailable();
    static bool RuntimeAvailable(std::string *reason = nullptr);
    static PipeWirePortalRuntimeState RuntimeState(std::string *reason = nullptr);
    static const char *RuntimeStateName(PipeWirePortalRuntimeState state);
};

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_PIPEWIRE_PORTAL_CAPTURE_H
