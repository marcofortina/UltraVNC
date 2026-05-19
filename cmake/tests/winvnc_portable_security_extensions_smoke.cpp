// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableSecurityExtensions.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::portable;

int main()
{
    SecurityExtensionDecision dsm = EvaluateSecurityExtension(ParseSecurityExtensionOption("--dsm-plugin"));
    assert(dsm.kind == SecurityExtensionKind::DsmPlugin);
    assert(!dsm.supported);
    assert(dsm.reason.find("DSMPlugin DLL ABI") != std::string::npos);
    assert(dsm.originalImplementation.find("DSMPlugin") != std::string::npos);

    SecurityExtensionDecision securevnc = EvaluateSecurityExtension(ParseSecurityExtensionOption("--securevnc-plugin"));
    assert(securevnc.kind == SecurityExtensionKind::SecureVncPlugin);
    assert(!securevnc.supported);
    assert(securevnc.wireType == rfbUltraVNC_SecureVNCPluginAuth_new);

    SecurityExtensionDecision mslogon = EvaluateSecurityExtension(ParseSecurityExtensionOption("--mslogon"));
    assert(mslogon.kind == SecurityExtensionKind::MsLogonII);
    assert(!mslogon.supported);
    assert(mslogon.wireType == rfbUltraVNC_MsLogonIIAuth);
    assert(mslogon.replacement.find("viewer can speak MSLogonII") != std::string::npos);

    SecurityExtensionDecision mslogonI = EvaluateSecurityExtension(ParseSecurityExtensionOption("--mslogon-i"));
    assert(mslogonI.kind == SecurityExtensionKind::MsLogonI);
    assert(mslogonI.wireType == rfbUltraVNC_MsLogonIAuth);

    SecurityExtensionDecision javaViewer = EvaluateSecurityExtension(ParseSecurityExtensionOption("--http-java-viewer"));
    assert(javaViewer.kind == SecurityExtensionKind::HttpJavaViewer);
    assert(!javaViewer.supported);
    assert(javaViewer.reason.find("legacy HTTP Java applet") != std::string::npos);
    assert(javaViewer.replacement.find("static/web viewer") != std::string::npos);
    return 0;
}
