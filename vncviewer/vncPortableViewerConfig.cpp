// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerConfig.h"

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
      updateIntervalMs_(1000)
{
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
    if (error) error->clear();
    return true;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
