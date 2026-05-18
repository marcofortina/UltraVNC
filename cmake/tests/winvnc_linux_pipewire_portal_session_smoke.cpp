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
    PipeWirePortalSessionRequest session = PipeWirePortalCaptureBackend::DefaultSessionRequest();
    PipeWirePortalSourceRequest sources = PipeWirePortalCaptureBackend::DefaultSourceRequest();

    std::string error;
    assert(PipeWirePortalCaptureBackend::ValidateSessionRequest(session, &error));
    assert(error.empty());
    assert(!session.sessionToken.empty());
    assert(!session.handleToken.empty());
    assert(session.requestCursorMetadata);
    assert(sources.screens);
    assert(!sources.windows);
    assert(!sources.virtualMonitors);

    session.sessionToken.clear();
    assert(!PipeWirePortalCaptureBackend::ValidateSessionRequest(session, &error));
    assert(error == "PipeWire/XDG portal session token must not be empty");

    session.sessionToken = std::string(129, 'x');
    session.handleToken = "uvnc_handle";
    assert(!PipeWirePortalCaptureBackend::ValidateSessionRequest(session, &error));
    assert(error == "PipeWire/XDG portal tokens are too long");
    return 0;
}
