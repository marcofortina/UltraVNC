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
    CaptureBackend resolved = CaptureBackend::X11;
    std::string error;

    assert(ResolveCaptureBackend(CaptureBackend::Auto, false, resolved, &error));
    assert(resolved == CaptureBackend::Memory);
    assert(error.empty());

    assert(ResolveCaptureBackend(CaptureBackend::Auto, true, resolved, &error));
    assert(resolved == CaptureBackend::RawFile);
    assert(error.empty());

    assert(ResolveCaptureBackend(CaptureBackend::Memory, false, resolved, &error));
    assert(resolved == CaptureBackend::Memory);

    assert(ResolveCaptureBackend(CaptureBackend::RawFile, true, resolved, &error));
    assert(resolved == CaptureBackend::RawFile);

    assert(!ResolveCaptureBackend(CaptureBackend::RawFile, false, resolved, &error));
    assert(error == "capture backend is not available: raw-file");

    assert(!ResolveCaptureBackend(CaptureBackend::X11, false, resolved, &error));
    assert(error == "capture backend is not available: x11");
    return 0;
}
