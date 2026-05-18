// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxPipeWirePortalCapture.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxfb;

int main()
{
    assert(std::string(PipeWirePortalCaptureBackend::RuntimeStateName(PipeWirePortalRuntimeState::Built)) == "available");
    assert(std::string(PipeWirePortalCaptureBackend::RuntimeStateName(PipeWirePortalRuntimeState::MissingBuildDependencies)) == "missing-build-dependencies");
    assert(std::string(PipeWirePortalCaptureBackend::RuntimeStateName(PipeWirePortalRuntimeState::MissingWaylandSession)) == "missing-wayland-session");
    assert(std::string(PipeWirePortalCaptureBackend::RuntimeStateName(PipeWirePortalRuntimeState::MissingPortalBus)) == "missing-portal-bus");

    std::string reason;
    const PipeWirePortalRuntimeState state = PipeWirePortalCaptureBackend::RuntimeState(&reason);
    assert(!reason.empty());
    assert(PipeWirePortalCaptureBackend::RuntimeAvailable() == (state == PipeWirePortalRuntimeState::Built));
    assert(!PipeWirePortalCaptureBackend::CaptureImplemented());
    return 0;
}
