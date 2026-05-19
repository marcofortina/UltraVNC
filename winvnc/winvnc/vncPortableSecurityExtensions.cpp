// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableSecurityExtensions.h"

namespace uvnc {
namespace winvnc {
namespace portable {

SecurityExtensionKind ParseSecurityExtensionOption(const std::string& option)
{
    if (option == "--security-plugin" || option == "--dsm-plugin") {
        return SecurityExtensionKind::DsmPlugin;
    }
    if (option == "--mslogon") {
        return SecurityExtensionKind::MsLogon;
    }
    if (option == "--http-java-viewer") {
        return SecurityExtensionKind::HttpJavaViewer;
    }
    return SecurityExtensionKind::Unknown;
}

SecurityExtensionDecision EvaluateSecurityExtension(SecurityExtensionKind kind)
{
    SecurityExtensionDecision decision;
    decision.kind = kind;
    decision.supported = false;
    switch (kind) {
    case SecurityExtensionKind::DsmPlugin:
        decision.reason = "DSM/security plugins are Windows DLL based and are not safely portable to the native Linux server path";
        decision.replacement = "use --transport-security vencrypt-x509-vnc with VNCAuth until a Linux-native plugin ABI is designed";
        break;
    case SecurityExtensionKind::MsLogon:
        decision.reason = "MSLogon depends on Windows account and domain APIs and is not available on Linux";
        decision.replacement = "use VNCAuth over VeNCrypt/TLS or add a future PAM-backed Linux authentication provider";
        break;
    case SecurityExtensionKind::HttpJavaViewer:
        decision.reason = "the legacy Java viewer endpoint is not part of the native Linux server runtime";
        decision.replacement = "serve viewers out-of-band or add a separately reviewed static web viewer endpoint";
        break;
    default:
        decision.reason = "unknown security extension";
        decision.replacement = "use documented native Linux security options";
        break;
    }
    return decision;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
