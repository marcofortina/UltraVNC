// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"

#include "rfb.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

ViewerCliOptions::ViewerCliOptions()
    : help(false),
      validateOnly(false),
      printConfig(false),
      smokeTest(false),
      connectSmoke(false),
      connectUpdateSmoke(false),
      connectDisplaySmoke(false),
      persistentInputSmoke(false),
      clipboardText("qt-viewer-clipboard"),
      listRemote(false),
      listRemoteDrives(false),
      downloadRemote(false),
      remoteChecksums(false),
      uploadLocal(false),
      remotePath(),
      downloadOutputPath(),
      uploadLocalPath()
{
}

namespace {

bool ReadTextFile(const std::string& path, std::string& text)
{
    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    text = buffer.str();
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    return true;
}

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

bool ParseEncodingList(const std::string& text, std::vector<unsigned int>& encodings)
{
    encodings.clear();
    std::string token;
    std::istringstream input(text);
    while (std::getline(input, token, ',')) {
        if (token == "raw") {
            encodings.push_back(rfbEncodingRaw);
        } else if (token == "copyrect") {
            encodings.push_back(rfbEncodingCopyRect);
        } else if (token == "hextile") {
            encodings.push_back(rfbEncodingHextile);
        } else if (token == "zlib") {
            encodings.push_back(rfbEncodingZlib);
        } else if (token == "zrle") {
            encodings.push_back(rfbEncodingZRLE);
        } else if (token == "tight") {
            encodings.push_back(rfbEncodingTight);
        } else if (token == "rre") {
            encodings.push_back(rfbEncodingRRE);
        } else if (token == "corre") {
            encodings.push_back(rfbEncodingCoRRE);
        } else if (token == "newfbsize") {
            encodings.push_back(rfbEncodingNewFBSize);
        } else if (token == "richcursor") {
            encodings.push_back(rfbEncodingRichCursor);
        } else if (token == "xcursor") {
            encodings.push_back(rfbEncodingXCursor);
        } else if (token == "pointerpos") {
            encodings.push_back(rfbEncodingPointerPos);
        } else if (token == "lastrect") {
            encodings.push_back(rfbEncodingLastRect);
        } else if (token == "extendedclipboard") {
            encodings.push_back(rfbEncodingExtendedClipboard);
        } else {
            return false;
        }
    }
    return !encodings.empty();
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
        } else if (arg == "--print-config") {
            options.printConfig = true;
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
        } else if (arg == "--persistent-input-smoke") {
            options.persistentInputSmoke = true;
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
        } else if (arg == "--allow-no-auth") {
            options.config.SetAllowNoAuth(true);
        } else if (arg == "--disable-no-auth") {
            options.config.SetAllowNoAuth(false);
        } else if (arg == "--transport-security" && i + 1 < args.size()) {
            ViewerTransportSecurityMode mode = ViewerTransportSecurityMode::None;
            if (!ParseViewerTransportSecurityMode(args[++i], mode)) {
                error = "invalid --transport-security";
                return false;
            }
            options.config.SetTransportSecurity(mode);
        } else if (arg == "--security-extension" && i + 1 < args.size()) {
            ViewerSecurityExtensionMode mode = ViewerSecurityExtensionMode::None;
            if (!ParseViewerSecurityExtensionMode(args[++i], mode)) {
                error = "invalid --security-extension";
                return false;
            }
            options.config.SetSecurityExtension(mode);
        } else if (arg == "--security-extension-name" && i + 1 < args.size()) {
            options.config.SetSecurityExtensionName(args[++i]);
        } else if (arg == "--tls-ca-file" && i + 1 < args.size()) {
            options.config.SetTlsCaFile(args[++i]);
        } else if (arg == "--tls-server-name" && i + 1 < args.size()) {
            options.config.SetTlsServerName(args[++i]);
        } else if (arg == "--tls-insecure") {
            options.config.SetTlsVerifyPeer(false);
        } else if (arg == "--username" && i + 1 < args.size()) {
            options.config.SetUsername(args[++i]);
        } else if (arg == "--password" && i + 1 < args.size()) {
            options.config.SetPassword(args[++i]);
        } else if (arg == "--password-file" && i + 1 < args.size()) {
            std::string password;
            if (!ReadTextFile(args[++i], password)) {
                error = "failed to read --password-file";
                return false;
            }
            options.config.SetPassword(password);
        } else if (arg == "--password-env" && i + 1 < args.size()) {
            const char *value = std::getenv(args[++i].c_str());
            if (!value) {
                error = "failed to read --password-env";
                return false;
            }
            options.config.SetPassword(value);
        } else if (arg == "--clipboard-text" && i + 1 < args.size()) {
            options.clipboardText = args[++i];
        } else if (arg == "--list-remote" && i + 1 < args.size()) {
            options.listRemote = true;
            options.remotePath = args[++i];
        } else if (arg == "--list-drives") {
            options.listRemoteDrives = true;
        } else if (arg == "--download-remote" && i + 1 < args.size()) {
            options.downloadRemote = true;
            options.remotePath = args[++i];
        } else if (arg == "--download-output" && i + 1 < args.size()) {
            options.downloadOutputPath = args[++i];
        } else if (arg == "--remote-checksums" && i + 1 < args.size()) {
            options.remoteChecksums = true;
            options.remotePath = args[++i];
        } else if (arg == "--upload-local" && i + 1 < args.size()) {
            options.uploadLocal = true;
            options.uploadLocalPath = args[++i];
        } else if (arg == "--upload-remote" && i + 1 < args.size()) {
            options.remotePath = args[++i];
        } else if (arg == "--continuous-updates") {
            options.config.SetContinuousUpdates(true);
        } else if (arg == "--encodings" && i + 1 < args.size()) {
            std::vector<unsigned int> encodings;
            if (!ParseEncodingList(args[++i], encodings)) {
                error = "invalid --encodings";
                return false;
            }
            options.config.SetEncodings(encodings);
        } else if (arg == "--socket-timeout-ms" && i + 1 < args.size()) {
            unsigned int timeout = 0;
            if (!ParseUnsigned(args[++i], timeout)) {
                error = "invalid --socket-timeout-ms";
                return false;
            }
            options.config.SetSocketTimeoutMs(timeout);
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

    const unsigned int fileTransferActions = (options.listRemote ? 1u : 0u) +
                                             (options.listRemoteDrives ? 1u : 0u) +
                                             (options.downloadRemote ? 1u : 0u) +
                                             (options.remoteChecksums ? 1u : 0u) +
                                             (options.uploadLocal ? 1u : 0u);
    if (fileTransferActions > 1) {
        error = "choose only one viewer file-transfer operation";
        return false;
    }
    if (options.downloadRemote && options.downloadOutputPath.empty()) {
        error = "--download-remote requires --download-output";
        return false;
    }
    if (options.uploadLocal && options.remotePath.empty()) {
        error = "--upload-local requires --upload-remote";
        return false;
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
        << "  --print-config         Print sanitized viewer configuration and exit\n"
        << "  --smoke-test           Create the Qt viewer shell and exit\n"
        << "  --connect-smoke        Connect to an RFB server, complete handshake, and exit\n"
        << "  --connect-update-smoke Connect to an RFB server, request one raw update, and exit\n"
        << "  --connect-display-smoke Connect, render one update into the Qt surface, and exit\n"
        << "  --persistent-input-smoke Connect once, send key/pointer events, request an update, and exit\n"
        << "  --host <host>          Viewer target host, default 127.0.0.1\n"
        << "  --port <port>          Viewer target port, default 5900\n"
        << "  --shared               Request shared session, default\n"
        << "  --exclusive            Request exclusive session\n"
        << "  --request-update       Request an initial framebuffer update in future session smoke\n"
        << "  --view-only            Disable local input forwarding in the viewer shell\n"
        << "  --allow-no-auth        Permit no-auth servers, default\n"
        << "  --disable-no-auth      Reject no-auth-only servers\n"
        << "  --transport-security <mode> Transport security: none, vencrypt-x509-vnc\n"
        << "  --security-extension <mode> Unsupported legacy extension request: none,dsm-plugin,mslogon,securevnc-plugin\n"
        << "  --security-extension-name <name> Legacy plugin/provider name for diagnostics only\n"
        << "  --tls-ca-file <path> CA file for VeNCrypt TLS peer verification\n"
        << "  --tls-server-name <name> Name used for TLS SNI and hostname verification\n"
        << "  --tls-insecure         Disable TLS peer verification for throwaway lab tests only\n"
        << "  --username <name>      Username for MSLogon-capable sessions\n"
        << "  --password <password>  Password for VNCAuth/MSLogon-capable sessions\n"
        << "  --password-file <path> Read VNCAuth password from a file\n"
        << "  --password-env <name>  Read VNCAuth password from an environment variable\n"
        << "  --clipboard-text <text> Clipboard text sent by persistent input smoke\n"
        << "  --list-remote <path> List a remote file-transfer directory and exit\n"
        << "  --list-drives       List remote file-transfer roots/drives and exit\n"
        << "  --download-remote <path> Download a remote file-transfer file and exit\n"
        << "  --download-output <path> Destination path for --download-remote\n"
        << "  --remote-checksums <path> Request remote file-transfer checksums and exit\n"
        << "  --upload-local <path> Upload a local file through remote file-transfer and exit\n"
        << "  --upload-remote <path> Remote destination path for --upload-local\n"
        << "  --encodings <list>     Comma-separated encodings: raw,copyrect,hextile,zlib,zrle,tight,rre,corre,newfbsize,richcursor,xcursor,pointerpos,lastrect,extendedclipboard\n"
        << "  --continuous-updates   Repeatedly request updates in the interactive Qt shell\n"
        << "  --socket-timeout-ms <ms> Socket read/write timeout, default 15000\n"
        << "  --update-interval-ms <ms> Continuous-update interval, default 1000\n";
    return out.str();
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
