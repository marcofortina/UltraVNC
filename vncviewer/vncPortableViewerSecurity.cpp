// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerSecurity.h"

#include <algorithm>
#include <sstream>

namespace uvnc {
namespace vncviewer {
namespace portable {

ViewerSecurityDecision::ViewerSecurityDecision()
    : selection(ViewerSecuritySelection::Unsupported), wireType(0), error()
{
}

const char *ViewerSecurityTypeName(CARD8 wireType)
{
    switch (wireType) {
    case rfbNoAuth:
        return "none";
    case rfbVncAuth:
        return "vnc-auth";
    case rfbVeNCypt:
        return "vencrypt";
    case rfbMSLogon:
        return "mslogon";
    case rfbUltraVNC:
        return "ultravnc";
    case rfbSSPI:
        return "sspi";
    case rfbSSPIMSLogon:
        return "sspi-mslogon";
    default:
        return "unknown";
    }
}

ViewerSecurityDecision SelectViewerSecurityType(const std::vector<CARD8>& serverTypes,
                                                bool hasPassword,
                                                bool allowNoAuth)
{
    ViewerSecurityDecision decision;
    const bool offersNoAuth = std::find(serverTypes.begin(), serverTypes.end(), rfbNoAuth) != serverTypes.end();
    const bool offersVncAuth = std::find(serverTypes.begin(), serverTypes.end(), rfbVncAuth) != serverTypes.end();
    const bool offersVeNCrypt = std::find(serverTypes.begin(), serverTypes.end(), rfbVeNCypt) != serverTypes.end();

    if (hasPassword && offersVncAuth) {
        decision.selection = ViewerSecuritySelection::VncAuth;
        decision.wireType = rfbVncAuth;
        return decision;
    }
    if (offersNoAuth && allowNoAuth) {
        decision.selection = ViewerSecuritySelection::NoAuth;
        decision.wireType = rfbNoAuth;
        return decision;
    }
    if (offersVncAuth) {
        decision.error = "RFB server requires VNCAuth but no password was provided";
        return decision;
    }
    if (offersNoAuth && !allowNoAuth) {
        decision.error = "RFB server only offers no-auth, but no-auth is disabled by viewer policy";
        return decision;
    }
    if (offersVeNCrypt) {
        decision.error = "RFB server offers VeNCrypt, but portable viewer TLS transport is not enabled in this build path";
        return decision;
    }

    std::ostringstream out;
    out << "RFB server does not offer a supported portable viewer security type";
    if (!serverTypes.empty()) {
        out << ":";
        for (std::size_t i = 0; i < serverTypes.size(); ++i) {
            out << ' ' << ViewerSecurityTypeName(serverTypes[i]) << '(' << static_cast<unsigned int>(serverTypes[i]) << ')';
        }
    }
    decision.error = out.str();
    return decision;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
