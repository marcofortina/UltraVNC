// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VIEWER_SECURITY_H
#define UVNC_VNCVIEWER_PORTABLE_VIEWER_SECURITY_H

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

enum class ViewerSecuritySelection {
    Unsupported,
    NoAuth,
    VncAuth
};

struct ViewerSecurityDecision {
    ViewerSecurityDecision();

    ViewerSecuritySelection selection;
    CARD8 wireType;
    std::string error;
};

ViewerSecurityDecision SelectViewerSecurityType(const std::vector<CARD8>& serverTypes,
                                                bool hasPassword,
                                                bool allowNoAuth = true);
const char *ViewerSecurityTypeName(CARD8 wireType);

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_SECURITY_H
