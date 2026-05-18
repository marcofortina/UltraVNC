// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VIEWER_CLI_H
#define UVNC_VNCVIEWER_PORTABLE_VIEWER_CLI_H

#include "vncPortableViewerConfig.h"

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

struct ViewerCliOptions {
    ViewerCliOptions();

    ViewerConfig config;
    bool help;
    bool validateOnly;
    bool smokeTest;
    bool connectSmoke;
    bool connectUpdateSmoke;
};

bool ParseViewerCli(const std::vector<std::string>& args, ViewerCliOptions& options, std::string& error);
std::string ViewerCliUsage(const char *programName);

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_CLI_H
