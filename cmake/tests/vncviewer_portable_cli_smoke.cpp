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
#include <cstdlib>
#include <fstream>
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
    args.push_back("--disable-no-auth");
    args.push_back("--password");
    args.push_back("secret");
    args.push_back("--transport-security");
    args.push_back("vencrypt-x509-vnc");
    args.push_back("--tls-server-name");
    args.push_back("example.test");
    args.push_back("--tls-insecure");
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
    assert(!options.config.AllowNoAuth());
    assert(options.smokeTest);
    assert(options.config.Encodings().size() == 14);
    assert(options.config.Encodings()[0] == rfbEncodingRaw);
    assert(options.config.Encodings()[1] == rfbEncodingCopyRect);
    assert(options.config.Encodings()[2] == rfbEncodingHextile);
    assert(options.config.Encodings()[3] == rfbEncodingZlib);
    assert(options.config.Encodings()[4] == rfbEncodingZRLE);
    assert(options.config.Encodings()[5] == rfbEncodingTight);
    assert(options.config.Password() == "secret");
    assert(options.config.TransportSecurity() == ViewerTransportSecurityMode::VeNCryptX509Vnc);
    assert(options.config.TlsServerName() == "example.test");
    assert(!options.config.TlsVerifyPeer());
    assert(options.config.ContinuousUpdates());
    assert(options.clipboardText == "hello clipboard");
    assert(options.config.UpdateIntervalMs() == 250);
    assert(options.config.SocketTimeoutMs() == 5000);
    assert(options.connectSmoke);
    assert(options.connectUpdateSmoke);
    assert(options.persistentInputSmoke);

    args.clear();
    args.push_back("--print-config");
    assert(ParseViewerCli(args, options, error));
    assert(options.printConfig);

    args.clear();
    args.push_back("--connect-display-smoke");
    assert(ParseViewerCli(args, options, error));
    assert(options.connectDisplaySmoke);
    assert(options.config.RequestUpdate());

    args.clear();
    args.push_back("--list-remote");
    args.push_back("/");
    assert(ParseViewerCli(args, options, error));
    assert(options.listRemote);
    assert(options.remotePath == "/");

    args.clear();
    args.push_back("--list-drives");
    assert(ParseViewerCli(args, options, error));
    assert(options.listRemoteDrives);

    args.clear();
    args.push_back("--upload-local");
    args.push_back("/tmp/local.txt");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "--upload-local requires --upload-remote");

    args.clear();
    args.push_back("--download-remote");
    args.push_back("remote.txt");
    args.push_back("--download-output");
    args.push_back("/tmp/uvnc-viewer-download.txt");
    assert(ParseViewerCli(args, options, error));
    assert(options.downloadRemote);
    assert(options.remotePath == "remote.txt");
    assert(options.downloadOutputPath == "/tmp/uvnc-viewer-download.txt");

    args.clear();
    args.push_back("--remote-checksums");
    args.push_back("remote.txt");
    assert(ParseViewerCli(args, options, error));
    assert(options.remoteChecksums);
    assert(options.remotePath == "remote.txt");

    args.clear();
    args.push_back("--upload-local");
    args.push_back("/tmp/local.txt");
    args.push_back("--upload-remote");
    args.push_back("remote.txt");
    assert(ParseViewerCli(args, options, error));
    assert(options.uploadLocal);
    assert(options.uploadLocalPath == "/tmp/local.txt");
    assert(options.remotePath == "remote.txt");

    args.clear();
    args.push_back("--upload-local");
    args.push_back("/tmp/local.txt");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "--upload-local requires --upload-remote");

    args.clear();
    args.push_back("--download-remote");
    args.push_back("remote.txt");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "--download-remote requires --download-output");

    args.clear();
    args.push_back("--list-drives");
    args.push_back("--remote-checksums");
    args.push_back("remote.txt");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "choose only one viewer file-transfer operation");

    args.clear();
    args.push_back("--security-extension");
    args.push_back("mslogon");
    args.push_back("--username");
    args.push_back("LAB\\alice");
    args.push_back("--password");
    args.push_back("secret");
    assert(ParseViewerCli(args, options, error));
    assert(options.config.SecurityExtension() == ViewerSecurityExtensionMode::MsLogon);
    assert(options.config.Username() == "LAB\\alice");

    args.clear();
    args.push_back("--security-extension");
    args.push_back("dsm-plugin");
    assert(!ParseViewerCli(args, options, error));
    assert(error.find("dsm-plugin") != std::string::npos);

    args.clear();
    args.push_back("--security-extension");
    args.push_back("bad-plugin");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --security-extension");

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


    {
        std::ofstream out("/tmp/uvnc-viewer-cli-password.txt");
        out << "file-secret\n";
    }
    args.clear();
    args.push_back("--password-file");
    args.push_back("/tmp/uvnc-viewer-cli-password.txt");
    assert(ParseViewerCli(args, options, error));
    assert(options.config.Password() == "file-secret");

    setenv("UVNC_VIEWER_CLI_TEST_PASSWORD", "env-secret", 1);
    args.clear();
    args.push_back("--password-env");
    args.push_back("UVNC_VIEWER_CLI_TEST_PASSWORD");
    assert(ParseViewerCli(args, options, error));
    assert(options.config.Password() == "env-secret");

    args.clear();
    args.push_back("--password-file");
    args.push_back("/tmp/does-not-exist-uvnc-password");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "failed to read --password-file");

    args.clear();
    args.push_back("--encodings");
    args.push_back("raw,bad");
    assert(!ParseViewerCli(args, options, error));
    assert(error == "invalid --encodings");

    const std::string usage = ViewerCliUsage("uvnc_qt_viewer");
    assert(usage.find("--view-only") != std::string::npos);
    assert(usage.find("--disable-no-auth") != std::string::npos);
    assert(usage.find("--connect-update-smoke") != std::string::npos);
    assert(usage.find("--persistent-input-smoke") != std::string::npos);
    assert(usage.find("--list-remote") != std::string::npos);
    assert(usage.find("--download-remote") != std::string::npos);
    assert(usage.find("--upload-local") != std::string::npos);
    assert(usage.find("--security-extension") != std::string::npos);
    assert(usage.find("--print-config") != std::string::npos);
    return 0;
}
