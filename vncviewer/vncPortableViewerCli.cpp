// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"

#include <cstdlib>
#include <sstream>

namespace uvnc {
namespace vncviewer {
namespace portable {

ViewerCliOptions::ViewerCliOptions()
    : help(false),
      validateOnly(false),
      smokeTest(false),
      connectSmoke(false),
      connectUpdateSmoke(false)
{
}

namespace {

bool ParsePort(const std::string& text, unsigned short& port)
{
    char *end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (!end || *end != '\0' || value <= 0 || value > 65535) {
        return false;
    }
    port = static_cast<unsigned short>(value);
    return true;
}

} // namespace

bool ParseViewerCli(const std::vector<std::string>& args, ViewerCliOptions& options, std::string& error)
{
    options = ViewerCliOptions();
    error.clear();

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--validate-config") {
            options.validateOnly = true;
        } else if (arg == "--smoke-test") {
            options.smokeTest = true;
        } else if (arg == "--connect-smoke") {
            options.connectSmoke = true;
        } else if (arg == "--connect-update-smoke") {
            options.connectUpdateSmoke = true;
            options.config.SetRequestUpdate(true);
        } else if (arg == "--host" && i + 1 < args.size()) {
            options.config.SetHost(args[++i]);
        } else if (arg == "--port" && i + 1 < args.size()) {
            unsigned short port = 0;
            if (!ParsePort(args[++i], port)) {
                error = "invalid --port";
                return false;
            }
            options.config.SetPort(port);
        } else if (arg == "--shared") {
            options.config.SetShared(true);
        } else if (arg == "--exclusive") {
            options.config.SetShared(false);
        } else if (arg == "--request-update") {
            options.config.SetRequestUpdate(true);
        } else if (arg == "--view-only") {
            options.config.SetViewOnly(true);
        } else {
            error = "unknown or incomplete option: " + arg;
            return false;
        }
    }

    return options.config.Validate(&error);
}

std::string ViewerCliUsage(const char *programName)
{
    std::ostringstream out;
    out << "Usage: " << programName << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --help                 Show this help and exit\n"
        << "  --validate-config      Validate viewer options and exit\n"
        << "  --smoke-test           Create the Qt viewer shell and exit\n"
        << "  --connect-smoke        Connect to an RFB server, complete handshake, and exit\n"
        << "  --connect-update-smoke Connect to an RFB server, request one raw update, and exit\n"
        << "  --host <host>          Viewer target host, default 127.0.0.1\n"
        << "  --port <port>          Viewer target port, default 5900\n"
        << "  --shared               Request shared session, default\n"
        << "  --exclusive            Request exclusive session\n"
        << "  --request-update       Request an initial framebuffer update in future session smoke\n"
        << "  --view-only            Disable local input forwarding in the viewer shell\n";
    return out.str();
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
