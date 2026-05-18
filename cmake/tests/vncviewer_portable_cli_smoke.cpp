// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"

#include <cassert>
#include <string>
#include <vector>

using namespace uvnc::vncviewer::portable;

int main()
{
    ViewerCliOptions options;
    std::string error;

    assert(ParseViewerCli(std::vector<std::string>(), options, error));
    assert(options.config.Host() == "127.0.0.1");
    assert(options.config.Port() == 5900);
    assert(options.config.Shared());

    std::vector<std::string> args;
    args.push_back("--host");
    args.push_back("example.test");
    args.push_back("--port");
    args.push_back("5901");
    args.push_back("--exclusive");
    args.push_back("--request-update");
    args.push_back("--view-only");
    args.push_back("--smoke-test");
    assert(ParseViewerCli(args, options, error));
    assert(options.config.Host() == "example.test");
    assert(options.config.Port() == 5901);
    assert(!options.config.Shared());
    assert(options.config.RequestUpdate());
    assert(options.config.ViewOnly());
    assert(options.smokeTest);

    args.clear();
    args.push_back("--port");
    args.push_back("70000");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --port");

    args.clear();
    args.push_back("--unknown");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "unknown or incomplete option: --unknown");

    const std::string usage = ViewerCliUsage("uvnc_qt_viewer");
    assert(usage.find("--view-only") != std::string::npos);
    return 0;
}
