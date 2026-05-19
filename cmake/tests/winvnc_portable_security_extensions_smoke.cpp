// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableSecurityExtensions.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    SecurityExtensionDecision dsm = EvaluateSecurityExtension(ParseSecurityExtensionOption("--dsm-plugin"));
    assert(dsm.kind == SecurityExtensionKind::DsmPlugin);
    assert(!dsm.supported);
    assert(dsm.reason.find("Windows DLL") != std::string::npos);
    assert(dsm.replacement.find("vencrypt") != std::string::npos || dsm.replacement.find("VeNCrypt") != std::string::npos);

    SecurityExtensionDecision mslogon = EvaluateSecurityExtension(ParseSecurityExtensionOption("--mslogon"));
    assert(mslogon.kind == SecurityExtensionKind::MsLogon);
    assert(!mslogon.supported);
    assert(mslogon.replacement.find("PAM") != std::string::npos);

    SecurityExtensionDecision javaViewer = EvaluateSecurityExtension(ParseSecurityExtensionOption("--http-java-viewer"));
    assert(javaViewer.kind == SecurityExtensionKind::HttpJavaViewer);
    assert(!javaViewer.supported);
    return 0;
}
