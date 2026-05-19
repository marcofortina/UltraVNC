// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableSecurityExtensions.h"

#include "rfb.h"

namespace uvnc {
namespace winvnc {
namespace portable {

SecurityExtensionKind ParseSecurityExtensionOption(const std::string& option)
{
    if (option == "--security-plugin" || option == "--dsm-plugin") {
        return SecurityExtensionKind::DsmPlugin;
    }
    if (option == "--securevnc-plugin") {
        return SecurityExtensionKind::SecureVncPlugin;
    }
    if (option == "--mslogon-i") {
        return SecurityExtensionKind::MsLogonI;
    }
    if (option == "--mslogon" || option == "--mslogon-ii") {
        return SecurityExtensionKind::MsLogonII;
    }
    if (option == "--http-java-viewer") {
        return SecurityExtensionKind::HttpJavaViewer;
    }
    return SecurityExtensionKind::Unknown;
}

const char *SecurityExtensionKindName(SecurityExtensionKind kind)
{
    switch (kind) {
    case SecurityExtensionKind::DsmPlugin:
        return "dsm-plugin";
    case SecurityExtensionKind::SecureVncPlugin:
        return "securevnc-plugin";
    case SecurityExtensionKind::MsLogonI:
        return "mslogon-i";
    case SecurityExtensionKind::MsLogonII:
        return "mslogon-ii";
    case SecurityExtensionKind::HttpJavaViewer:
        return "http-java-viewer";
    default:
        return "unknown";
    }
}

SecurityExtensionDecision EvaluateSecurityExtension(SecurityExtensionKind kind)
{
    SecurityExtensionDecision decision;
    decision.kind = kind;
    decision.supported = false;
    decision.wireType = 0;
    switch (kind) {
    case SecurityExtensionKind::DsmPlugin:
        decision.reason = "DSM plugins use the legacy Windows DSMPlugin DLL ABI and transform the stream below RFB";
        decision.replacement = "use VeNCrypt/X509Vnc or design a Linux-native stream provider ABI";
        decision.originalImplementation = "DSMPlugin/DSMPlugin.cpp; winvnc/winvnc/vsocket.cpp; vncviewer/VNCOptions.cpp";
        break;
    case SecurityExtensionKind::SecureVncPlugin:
        decision.reason = "SecureVNC is distributed as a DSM plugin and depends on the DSMPlugin ABI";
        decision.replacement = "use VeNCrypt/X509Vnc until a native Linux DSM-compatible provider exists";
        decision.wireType = rfbUltraVNC_SecureVNCPluginAuth_new;
        decision.originalImplementation = "winvnc/winvnc/res/SecureVNCPlugin.dsm; vncclient AuthSecureVNCPlugin";
        break;
    case SecurityExtensionKind::MsLogonI:
        decision.reason = "MSLogon I is legacy and depends on Windows/domain account semantics";
        decision.replacement = "use MSLogonII with the Linux external auth helper for native server deployments";
        decision.wireType = rfbUltraVNC_MsLogonIAuth;
        decision.originalImplementation = "vncviewer/ClientConnection.cpp AuthMsLogonI; winvnc/winvnc/vncntlm.cpp";
        break;
    case SecurityExtensionKind::MsLogonII:
        decision.reason = "server-side MSLogonII verification depends on Windows/domain account semantics";
        decision.replacement = "viewer can speak MSLogonII and Linux server supports MSLogonII with an external auth helper";
        decision.wireType = rfbUltraVNC_MsLogonIIAuth;
        decision.originalImplementation = "vncviewer/ClientConnection.cpp AuthMsLogonII; winvnc/winvnc/vncclient.cpp AuthMsLogon";
        break;
    case SecurityExtensionKind::HttpJavaViewer:
        decision.reason = "the legacy HTTP Java applet viewer endpoint is not part of the native Linux server runtime";
        decision.replacement = "use native VNC viewers or add a separately reviewed static/web viewer endpoint instead of the legacy applet";
        decision.originalImplementation = "winvnc/winvnc/vnchttpconnect.cpp; JavaViewer/";
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
