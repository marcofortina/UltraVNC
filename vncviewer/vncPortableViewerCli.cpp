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
      connectUpdateSmoke(false),
      connectDisplaySmoke(false)
{
}

namespace {

bool ParseUnsigned(const std::string& text, unsigned int& value)
{
    char *end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (!end || *end != '\0' || parsed <= 0) {
        return false;
    }
    value = static_cast<unsigned int>(parsed);
    return true;
}

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
        } else if (arg == "--connect-display-smoke") {
            options.connectDisplaySmoke = true;
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
        } else if (arg == "--password" && i + 1 < args.size()) {
            options.config.SetPassword(args[++i]);
        } else if (arg == "--continuous-updates") {
            options.config.SetContinuousUpdates(true);
        } else if (arg == "--update-interval-ms" && i + 1 < args.size()) {
            unsigned int interval = 0;
            if (!ParseUnsigned(args[++i], interval)) {
                error = "invalid --update-interval-ms";
                return false;
            }
            options.config.SetUpdateIntervalMs(interval);
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
        << "  --connect-display-smoke Connect, render one update into the Qt surface, and exit\n"
        << "  --host <host>          Viewer target host, default 127.0.0.1\n"
        << "  --port <port>          Viewer target port, default 5900\n"
        << "  --shared               Request shared session, default\n"
        << "  --exclusive            Request exclusive session\n"
        << "  --request-update       Request an initial framebuffer update in future session smoke\n"
        << "  --view-only            Disable local input forwarding in the viewer shell\n"
        << "  --password <password>  Password for future VNCAuth-capable sessions\n"
        << "  --continuous-updates   Repeatedly request updates in the interactive Qt shell\n"
        << "  --update-interval-ms <ms> Continuous-update interval, default 1000\n";
    return out.str();
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
