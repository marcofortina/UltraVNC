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
    PipeWireStreamDescriptor descriptor = PipeWirePortalCaptureBackend::DefaultStreamDescriptor();
    std::string error;
    assert(!PipeWirePortalCaptureBackend::ValidateStreamDescriptor(descriptor, &error));
    assert(error == "PipeWire stream node id must be non-zero");

    descriptor.nodeId = 7;
    assert(!PipeWirePortalCaptureBackend::ValidateStreamDescriptor(descriptor, &error));
    assert(error == "PipeWire stream dimensions must be non-zero");

    descriptor.width = 640;
    descriptor.height = 480;
    descriptor.bytesPerPixel = 4;
    descriptor.stride = 640 * 4 - 1;
    assert(!PipeWirePortalCaptureBackend::ValidateStreamDescriptor(descriptor, &error));
    assert(error == "PipeWire stream stride is smaller than one row");

    descriptor.stride = 640 * 4;
    assert(PipeWirePortalCaptureBackend::ValidateStreamDescriptor(descriptor, &error));
    assert(error.empty());
    return 0;
}
