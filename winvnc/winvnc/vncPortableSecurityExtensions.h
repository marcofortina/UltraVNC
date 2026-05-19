// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_SECURITY_EXTENSIONS_H
#define UVNC_WINVNC_PORTABLE_SECURITY_EXTENSIONS_H

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

enum class SecurityExtensionKind {
    DsmPlugin,
    MsLogon,
    HttpJavaViewer,
    Unknown,
};

struct SecurityExtensionDecision {
    SecurityExtensionKind kind;
    bool supported;
    std::string reason;
    std::string replacement;
};

SecurityExtensionKind ParseSecurityExtensionOption(const std::string& option);
SecurityExtensionDecision EvaluateSecurityExtension(SecurityExtensionKind kind);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_SECURITY_EXTENSIONS_H
