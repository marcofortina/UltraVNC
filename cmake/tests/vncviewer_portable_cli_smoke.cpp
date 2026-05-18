// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"

#include "rfb.h"

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
    args.push_back("--password");
    args.push_back("secret");
    args.push_back("--continuous-updates");
    args.push_back("--clipboard-text");
    args.push_back("hello clipboard");
    args.push_back("--update-interval-ms");
    args.push_back("250");
    args.push_back("--socket-timeout-ms");
    args.push_back("5000");
    args.push_back("--smoke-test");
    args.push_back("--connect-smoke");
    args.push_back("--connect-update-smoke");
    args.push_back("--persistent-input-smoke");
    assert(ParseViewerCli(args, options, error));
    assert(options.config.Host() == "example.test");
    assert(options.config.Port() == 5901);
    assert(!options.config.Shared());
    assert(options.config.RequestUpdate());
    assert(options.config.ViewOnly());
    assert(options.smokeTest);
    assert(options.config.Encodings().size() == 7);
    assert(options.config.Encodings()[0] == rfbEncodingRaw);
    assert(options.config.Encodings()[1] == rfbEncodingCopyRect);
    assert(options.config.Encodings()[2] == rfbEncodingHextile);
    assert(options.config.Encodings()[3] == rfbEncodingZlib);
    assert(options.config.Password() == "secret");
    assert(options.config.ContinuousUpdates());
    assert(options.clipboardText == "hello clipboard");
    assert(options.config.UpdateIntervalMs() == 250);
    assert(options.config.SocketTimeoutMs() == 5000);
    assert(options.connectSmoke);
    assert(options.connectUpdateSmoke);
    assert(options.persistentInputSmoke);

    args.clear();
    args.push_back("--connect-display-smoke");
    assert(ParseViewerCli(args, options, error));
    assert(options.connectDisplaySmoke);
    assert(options.config.RequestUpdate());

    args.clear();
    args.push_back("--port");
    args.push_back("70000");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --port");

    args.clear();
    args.push_back("--socket-timeout-ms");
    args.push_back("0");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --socket-timeout-ms");

    args.clear();
    args.push_back("--update-interval-ms");
    args.push_back("0");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --update-interval-ms");

    args.clear();
    args.push_back("--unknown");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "unknown or incomplete option: --unknown");

    args.clear();
    args.push_back("--encodings");
    args.push_back("raw,bad");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --encodings");

    const std::string usage = ViewerCliUsage("uvnc_qt_viewer");
    assert(usage.find("--view-only") != std::string::npos);
    assert(usage.find("--connect-update-smoke") != std::string::npos);
    assert(usage.find("--persistent-input-smoke") != std::string::npos);
    return 0;
}
