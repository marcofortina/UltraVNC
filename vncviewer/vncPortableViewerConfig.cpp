// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerConfig.h"

#include "rfb.h"

namespace uvnc {
namespace vncviewer {
namespace portable {

ViewerConfig::ViewerConfig()
    : host_("127.0.0.1"),
      port_(5900),
      shared_(true),
      requestUpdate_(false),
      viewOnly_(false),
      password_(),
      continuousUpdates_(false),
      updateIntervalMs_(1000),
      encodings_(),
      socketTimeoutMs_(15000)
{
    encodings_.push_back(rfbEncodingRaw);
    encodings_.push_back(rfbEncodingCopyRect);
    encodings_.push_back(rfbEncodingHextile);
    encodings_.push_back(rfbEncodingZlib);
    encodings_.push_back(rfbEncodingZRLE);
    encodings_.push_back(rfbEncodingTight);
    encodings_.push_back(rfbEncodingRRE);
    encodings_.push_back(rfbEncodingCoRRE);
    encodings_.push_back(rfbEncodingNewFBSize);
    encodings_.push_back(rfbEncodingRichCursor);
    encodings_.push_back(rfbEncodingXCursor);
    encodings_.push_back(rfbEncodingPointerPos);
    encodings_.push_back(rfbEncodingLastRect);
    encodings_.push_back(rfbEncodingExtendedClipboard);
}

bool ViewerConfig::Validate(std::string *error) const
{
    if (host_.empty()) {
        if (error) *error = "viewer host must not be empty";
        return false;
    }
    if (port_ == 0) {
        if (error) *error = "viewer port must not be zero";
        return false;
    }
    if (updateIntervalMs_ == 0) {
        if (error) *error = "viewer update interval must not be zero";
        return false;
    }
    if (socketTimeoutMs_ == 0) {
        if (error) *error = "viewer socket timeout must not be zero";
        return false;
    }
    if (encodings_.empty()) {
        if (error) *error = "viewer encoding list must not be empty";
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
