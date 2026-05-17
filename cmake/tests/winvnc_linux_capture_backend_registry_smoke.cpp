// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxCaptureBackend.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxfb;

int main()
{
    CaptureBackend backend = CaptureBackend::Auto;
    assert(ParseCaptureBackendName("auto", backend) && backend == CaptureBackend::Auto);
    assert(ParseCaptureBackendName("memory", backend) && backend == CaptureBackend::Memory);
    assert(ParseCaptureBackendName("raw-file", backend) && backend == CaptureBackend::RawFile);
    assert(ParseCaptureBackendName("x11", backend) && backend == CaptureBackend::X11);
    assert(!ParseCaptureBackendName("pipewire", backend));

    assert(std::string(CaptureBackendName(CaptureBackend::Auto)) == "auto");
    assert(std::string(CaptureBackendName(CaptureBackend::Memory)) == "memory");
    assert(std::string(CaptureBackendName(CaptureBackend::RawFile)) == "raw-file");
    assert(std::string(CaptureBackendName(CaptureBackend::X11)) == "x11");
    assert(CaptureBackendDescription(CaptureBackend::Memory)[0] != '\0');

    assert(IsCaptureBackendRuntimeAvailable(CaptureBackend::Memory, false));
    assert(!IsCaptureBackendRuntimeAvailable(CaptureBackend::RawFile, false));
    assert(IsCaptureBackendRuntimeAvailable(CaptureBackend::RawFile, true));
    const bool x11Available = IsCaptureBackendRuntimeAvailable(CaptureBackend::X11, true);
    (void)x11Available;
    return 0;
}
