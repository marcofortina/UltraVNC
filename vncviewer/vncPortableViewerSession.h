// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VIEWER_SESSION_H
#define UVNC_VNCVIEWER_PORTABLE_VIEWER_SESSION_H

#include "vncPortableViewerConfig.h"

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

struct ViewerFramebufferUpdate {
    ViewerFramebufferUpdate();

    bool received;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    CARD32 encoding;
    std::vector<CARD8> pixels;
};

struct ViewerSessionResult {
    ViewerSessionResult();

    unsigned int width;
    unsigned int height;
    rfbPixelFormat format;
    std::string desktopName;
    ViewerFramebufferUpdate update;
};

class ViewerSession {
public:
    bool RunHandshake(const ViewerConfig& config, ViewerSessionResult& result, std::string *error = nullptr) const;
    bool RequestOneFramebufferUpdate(const ViewerConfig& config, ViewerSessionResult& result, std::string *error = nullptr) const;
};

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_SESSION_H
