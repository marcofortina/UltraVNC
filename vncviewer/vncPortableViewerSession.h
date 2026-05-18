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
#include "vncPortableTcp.h"

#include <string>
#include <vector>

#include <zlib.h>

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
    unsigned int sourceX;
    unsigned int sourceY;
    CARD32 encoding;
    std::vector<CARD8> pixels;
};

struct ViewerFramebufferRect {
    ViewerFramebufferRect();

    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned int sourceX;
    unsigned int sourceY;
    CARD32 encoding;
    std::vector<CARD8> pixels;
};


struct ViewerSessionResult {
    ViewerSessionResult();

    unsigned int width;
    unsigned int height;
    rfbPixelFormat format;
    std::string desktopName;
    std::string serverCutText;
    unsigned int bellCount;
    ViewerFramebufferUpdate update;
    std::vector<ViewerFramebufferRect> rectangles;
};

class ViewerSession {
public:
    bool RunHandshake(const ViewerConfig& config, ViewerSessionResult& result, std::string *error = nullptr) const;
    bool RequestOneFramebufferUpdate(const ViewerConfig& config, ViewerSessionResult& result, std::string *error = nullptr) const;
};

class PersistentViewerSession {
public:
    PersistentViewerSession();
    PersistentViewerSession(const PersistentViewerSession&) = delete;
    PersistentViewerSession& operator=(const PersistentViewerSession&) = delete;
    ~PersistentViewerSession();

    bool Connect(const ViewerConfig& config, ViewerSessionResult& result, std::string *error = nullptr);
    bool Connected() const;
    void Disconnect();

    bool RequestFramebufferUpdate(bool incremental, ViewerSessionResult& result, std::string *error = nullptr);
    bool SendKeyEvent(CARD32 keysym, bool down, std::string *error = nullptr);
    bool SendPointerEvent(CARD8 buttonMask, unsigned int x, unsigned int y, std::string *error = nullptr);
    bool SendClientCutText(const std::string& text, std::string *error = nullptr);

private:
    uvnc::winvnc::portable::TcpSocket socket_;
    ViewerSessionResult state_;
    ViewerConfig config_;
    std::vector<CARD8> framebuffer_;
    z_stream zrleStream_;
    bool zrleStreamInitialized_;
};

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_SESSION_H
