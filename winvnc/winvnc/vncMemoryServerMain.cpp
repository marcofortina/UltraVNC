// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxCaptureBackend.h"
#include "vncLinuxFramebufferSource.h"
#include "vncLinuxInputBackend.h"
#include "vncLinuxXTestInput.h"
#include "vncLinuxPipeWirePortalCapture.h"
#include "vncLinuxX11FramebufferSource.h"
#include "vncPortableFramebufferPattern.h"
#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include "d3des.h"
}

using uvnc::winvnc::portable::ServerConfig;
using uvnc::winvnc::portable::ServerAuthMode;
using uvnc::winvnc::portable::ServerAuthModeName;
using uvnc::winvnc::portable::ParseServerAuthMode;
using uvnc::winvnc::linuxfb::CaptureBackend;
using uvnc::winvnc::linuxfb::CaptureBackendName;
using uvnc::winvnc::linuxfb::LoadRawFramebufferFile;
using uvnc::winvnc::linuxfb::PipeWirePortalCaptureBackend;
using uvnc::winvnc::linuxfb::PipeWirePortalRuntimeState;
using uvnc::winvnc::linuxfb::ParseCaptureBackendName;
using uvnc::winvnc::linuxfb::ResolveCaptureBackend;
using uvnc::winvnc::linuxfb::X11DesktopSource;
using uvnc::winvnc::linuxinput::InputBackend;
using uvnc::winvnc::linuxinput::InputBackendName;
using uvnc::winvnc::linuxinput::ParseInputBackendName;
using uvnc::winvnc::linuxinput::ResolveInputBackend;
using uvnc::winvnc::linuxinput::XTestInputBackend;
using uvnc::winvnc::portable::DesktopSource;
using uvnc::winvnc::portable::Framebuffer;
using uvnc::winvnc::portable::MemoryServer;
using uvnc::winvnc::portable::FramebufferUpdateRequest;
using uvnc::winvnc::portable::KeyEvent;
using uvnc::winvnc::portable::PointerEvent;
using uvnc::winvnc::portable::RfbInputSink;
using uvnc::winvnc::portable::TcpSocket;
using uvnc::winvnc::portable::FramebufferPattern;
using uvnc::winvnc::portable::FramebufferPatternName;
using uvnc::winvnc::portable::ParseFramebufferPattern;

namespace {

volatile std::sig_atomic_t g_stopRequested = 0;


void HandleStopSignal(int)
{
    g_stopRequested = 1;
}

bool StopRequested()
{
    return g_stopRequested != 0;
}

void InstallStopSignalHandlers()
{
    std::signal(SIGINT, HandleStopSignal);
    std::signal(SIGTERM, HandleStopSignal);
}

bool IsLoopbackBindAddress(const std::string& address)
{
    return address == "localhost" || address.rfind("127.", 0) == 0;
}

bool IsAllInterfacesBindAddress(const std::string& address)
{
    return address == "0.0.0.0";
}

bool ValidateRegularFilePermissions(const std::string& path, bool requirePrivate, std::string *error)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        if (error) *error = "cannot stat file: " + path;
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        if (error) *error = "not a regular file: " + path;
        return false;
    }
    if (st.st_mode & (S_IWGRP | S_IWOTH)) {
        if (error) *error = "file must not be group/world writable: " + path;
        return false;
    }
    if (requirePrivate && (st.st_mode & (S_IRWXG | S_IRWXO))) {
        if (error) *error = "password file must not be accessible by group/other: " + path;
        return false;
    }
    return true;
}

bool ReadPasswordFile(const std::string& path, std::string& password, std::string *error)
{
    if (!ValidateRegularFilePermissions(path, true, error)) {
        return false;
    }
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        if (error) *error = "cannot open password file: " + path;
        return false;
    }
    std::getline(input, password);
    while (!password.empty() && (password[password.size() - 1] == '\r' || password[password.size() - 1] == '\n')) {
        password.resize(password.size() - 1);
    }
    if (password.empty()) {
        if (error) *error = "password file is empty: " + path;
        return false;
    }
    if (password.size() > 8) {
        if (error) *error = "VNCAuth password file value must be at most 8 bytes: " + path;
        return false;
    }
    return true;
}

bool EnforceLinuxServerSecurityPolicy(const ServerConfig& config, std::string *error)
{
    const bool loopback = IsLoopbackBindAddress(config.BindAddress());
    if (config.AuthMode() == ServerAuthMode::NoAuth) {
        if (!config.AllowNoAuth()) {
            if (error) *error = "no-auth is disabled by default; use --allow-no-auth only for loopback/lab use or configure --auth vnc-password --password-file";
            return false;
        }
        if (!loopback && !config.AllowPublicNoAuth()) {
            if (error) *error = "refusing no-auth on a non-loopback bind address; configure VNCAuth or use --allow-public-no-auth only for controlled lab validation";
            return false;
        }
    }
    if (config.AuthMode() == ServerAuthMode::VncPassword && !loopback && !config.AllowUnencryptedPublic()) {
        if (error) *error = "refusing VNCAuth on a non-loopback bind without transport encryption; use loopback/tunnel/firewall or --allow-unencrypted-public for explicit controlled exposure";
        return false;
    }
    return true;
}

void HardenRuntimeFileCreationUmask()
{
    // Runtime pid, status and log files can contain operational details.
    // Keep newly-created files private even when the parent process has a
    // permissive umask.
    umask(S_IRWXG | S_IRWXO);
}

void PrintSecurityWarnings(const ServerConfig& config)
{
    if (IsAllInterfacesBindAddress(config.BindAddress())) {
        std::cerr << "warning: listening on 0.0.0.0; this experimental Linux server has no transport encryption yet" << "\n";
    }
    if (config.AuthMode() == ServerAuthMode::VncPassword) {
        std::cerr << "warning: VNCAuth protects the handshake but does not encrypt framebuffer/input traffic" << "\n";
    }
}

bool WriteTextFile(const std::string& path, const std::string& value)
{
    if (path.empty()) {
        return true;
    }
    std::ofstream out(path.c_str(), std::ios::trunc);
    if (!out) {
        return false;
    }
    out << value;
    return static_cast<bool>(out);
}

void RemoveFileIfSet(const std::string& path)
{
    if (!path.empty()) {
        std::remove(path.c_str());
    }
}



bool ValidateRuntimeFilePath(const std::string& path, const char *label, std::string *error)
{
    if (path.empty()) {
        return true;
    }
    if (path[path.size() - 1] == '/') {
        if (error) *error = std::string(label) + " path must be a file path, not a directory: " + path;
        return false;
    }

    const std::size_t slash = path.find_last_of('/');
    const std::string parent = slash == std::string::npos ? "." : (slash == 0 ? "/" : path.substr(0, slash));
    struct stat parentStat;
    if (stat(parent.c_str(), &parentStat) != 0 || !S_ISDIR(parentStat.st_mode)) {
        if (error) *error = std::string(label) + " parent directory does not exist: " + parent;
        return false;
    }
    if (access(parent.c_str(), W_OK | X_OK) != 0) {
        if (error) *error = std::string(label) + " parent directory is not writable/searchable: " + parent;
        return false;
    }

    struct stat fileStat;
    if (lstat(path.c_str(), &fileStat) == 0) {
        if (S_ISLNK(fileStat.st_mode)) {
            if (error) *error = std::string(label) + " path must not be a symlink: " + path;
            return false;
        }
        if (S_ISDIR(fileStat.st_mode)) {
            if (error) *error = std::string(label) + " path must not be a directory: " + path;
            return false;
        }
        if (!S_ISREG(fileStat.st_mode)) {
            if (error) *error = std::string(label) + " path must be a regular file when it already exists: " + path;
            return false;
        }
        if (fileStat.st_mode & (S_IWGRP | S_IWOTH)) {
            if (error) *error = std::string(label) + " file must not be group/world writable: " + path;
            return false;
        }
    }
    return true;
}

bool ValidateRuntimeFilePaths(const std::string& pidFile, const std::string& statusFile, const std::string& logFile, std::string *error)
{
    return ValidateRuntimeFilePath(pidFile, "pid-file", error) &&
           ValidateRuntimeFilePath(statusFile, "status-file", error) &&
           ValidateRuntimeFilePath(logFile, "log-file", error);
}

class ScopedStreamBufferRedirect {
public:
    ScopedStreamBufferRedirect()
        : stream_(nullptr), original_(nullptr)
    {
    }

    ScopedStreamBufferRedirect(std::ostream& stream, std::streambuf *replacement)
        : stream_(&stream), original_(stream.rdbuf(replacement))
    {
    }

    ScopedStreamBufferRedirect(const ScopedStreamBufferRedirect&) = delete;
    ScopedStreamBufferRedirect& operator=(const ScopedStreamBufferRedirect&) = delete;

    ~ScopedStreamBufferRedirect()
    {
        Restore();
    }

    void Redirect(std::ostream& stream, std::streambuf *replacement)
    {
        Restore();
        stream_ = &stream;
        original_ = stream.rdbuf(replacement);
    }

    void Restore()
    {
        if (stream_ != nullptr) {
            stream_->rdbuf(original_);
            stream_ = nullptr;
            original_ = nullptr;
        }
    }

private:
    std::ostream *stream_;
    std::streambuf *original_;
};

class XTestRfbInputSink : public RfbInputSink {
public:
    bool InjectKey(const KeyEvent& event, std::string *error) override
    {
        return input_.InjectKeySym(event.keysym, event.down, error);
    }

    bool InjectPointer(const PointerEvent& event, std::string *error) override
    {
        return input_.InjectPointer(event.buttonMask, event.x, event.y, error);
    }

private:
    XTestInputBackend input_;
};

void PrintUsage(const char *name)
{
    std::cout << "Usage: " << name << " [options]\n"
              << "\n"
              << "Experimental native Linux WinVNC memory server.\n"
              << "This target serves an in-memory framebuffer skeleton and does not capture a real desktop yet.\n"
              << "\n"
              << "Options:\n"
              << "  --bind-address <ipv4>   Bind address, default 127.0.0.1\n"
              << "  --port <port>           RFB port, default 0 for ephemeral\n"
              << "  --width <pixels>        Framebuffer width, default 640\n"
              << "  --height <pixels>       Framebuffer height, default 480\n"
              << "  --name <text>           Desktop name\n"
              << "  --fill-byte <0-255>    Fill byte for the in-memory framebuffer, default 34\n"
              << "  --pattern <name>       Framebuffer pattern: solid, checker, gradient-x, gradient-y\n"
              << "  --capture-backend <name> Capture backend: auto, memory, raw-file, x11, pipewire\n"
              << "  --input-backend <name> Input backend: auto, none, xtest\n"
              << "  --raw-framebuffer-file <path> Serve exact-size raw framebuffer file instead of synthetic pattern\n"
              << "  --config <path>         Load key=value server runtime config before CLI overrides\n"
              << "  --auth <mode>           Auth mode: none or vnc-password\n"
              << "  --password-file <path>  Read VNCAuth password from a private file, max 8 bytes\n"
              << "  --allow-no-auth         Explicitly allow no-auth loopback/lab mode\n"
              << "  --allow-public-no-auth  Explicitly allow no-auth on non-loopback lab binds\n"
              << "  --allow-unencrypted-public Explicitly allow non-loopback VNCAuth without transport encryption\n"
              << "  --validate-config       Validate options and exit\n"
              << "  --print-config          Print resolved configuration and exit\n"
              << "  --smoke-test            Start on loopback, complete one RFB handshake, and exit\n"
              << "  --smoke-update-test     Start on loopback, request one raw framebuffer update, and exit\n"
              << "  --smoke-multi-update-test Start on loopback, request multiple raw framebuffer updates, and exit\n"
              << "  --smoke-raw-file-update-test Start on loopback using a raw framebuffer file, request one update, and exit\n"
              << "  --smoke-x11-update-test Start on loopback using an X11 snapshot, request one update, and exit\n"
              << "  --smoke-x11-availability-test Print X11/XShm runtime availability and exit\n"
              << "  --smoke-pipewire-availability-test Print PipeWire/XDG portal runtime availability and exit\n"
              << "  --smoke-xtest-availability-test Print XTest input runtime availability and exit\n"
              << "  --smoke-xtest-input-test Inject a minimal XTest key/pointer sequence when explicitly allowed\n"
              << "  --allow-input-injection Allow live input injection smoke tests\n"
              << "  --max-updates <count>  Number of updates for multi-update smoke/serve mode, default 3\n"
              << "  --serve-updates        Serve one client through --max-updates framebuffer updates\n"
              << "  --serve-forever        Keep accepting update clients until SIGINT/SIGTERM\n"
              << "  --pid-file <path>      Write process id while the server is running\n"
              << "  --status-file <path>   Write coarse runtime status transitions\n"
              << "  --log-file <path>      Append stdout/stderr logs to a file\n"
              << "  --help                  Show this help\n";
}

std::string Trim(const std::string& value)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t begin = value.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return std::string();
    }
    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(begin, end - begin + 1);
}

bool AddConfigOption(const std::string& key, const std::string& value, std::vector<std::string>& args, std::string *error)
{
    if (key == "bind_address") {
        args.push_back("--bind-address");
    } else if (key == "port") {
        args.push_back("--port");
    } else if (key == "width") {
        args.push_back("--width");
    } else if (key == "height") {
        args.push_back("--height");
    } else if (key == "name") {
        args.push_back("--name");
    } else if (key == "fill_byte") {
        args.push_back("--fill-byte");
    } else if (key == "pattern") {
        args.push_back("--pattern");
    } else if (key == "capture_backend") {
        args.push_back("--capture-backend");
    } else if (key == "input_backend") {
        args.push_back("--input-backend");
    } else if (key == "raw_framebuffer_file") {
        args.push_back("--raw-framebuffer-file");
    } else if (key == "auth") {
        args.push_back("--auth");
    } else if (key == "password_file") {
        args.push_back("--password-file");
    } else if (key == "allow_no_auth") {
        if (value == "true" || value == "1" || value == "yes") {
            args.push_back("--allow-no-auth");
            return true;
        }
        if (value == "false" || value == "0" || value == "no") {
            return true;
        }
        if (error) *error = "invalid boolean value for allow_no_auth";
        return false;
    } else if (key == "allow_public_no_auth") {
        if (value == "true" || value == "1" || value == "yes") {
            args.push_back("--allow-public-no-auth");
            return true;
        }
        if (value == "false" || value == "0" || value == "no") {
            return true;
        }
        if (error) *error = "invalid boolean value for allow_public_no_auth";
        return false;
    } else if (key == "allow_unencrypted_public") {
        if (value == "true" || value == "1" || value == "yes") {
            args.push_back("--allow-unencrypted-public");
            return true;
        }
        if (value == "false" || value == "0" || value == "no") {
            return true;
        }
        if (error) *error = "invalid boolean value for allow_unencrypted_public";
        return false;
    } else if (key == "max_updates") {
        args.push_back("--max-updates");
    } else if (key == "pid_file") {
        args.push_back("--pid-file");
    } else if (key == "status_file") {
        args.push_back("--status-file");
    } else if (key == "log_file") {
        args.push_back("--log-file");
    } else if (key == "serve_updates") {
        if (value == "true" || value == "1" || value == "yes") {
            args.push_back("--serve-updates");
            return true;
        }
        if (value == "false" || value == "0" || value == "no") {
            return true;
        }
        if (error) *error = "invalid boolean value for serve_updates";
        return false;
    } else if (key == "serve_forever") {
        if (value == "true" || value == "1" || value == "yes") {
            args.push_back("--serve-forever");
            return true;
        }
        if (value == "false" || value == "0" || value == "no") {
            return true;
        }
        if (error) *error = "invalid boolean value for serve_forever";
        return false;
    } else {
        if (error) *error = "unknown config key: " + key;
        return false;
    }

    if (value.empty()) {
        if (error) *error = "empty value for config key: " + key;
        return false;
    }
    args.push_back(value);
    return true;
}

bool AppendConfigFileArgs(const std::string& path, std::vector<std::string>& args, std::string *error)
{
    if (!ValidateRegularFilePermissions(path, false, error)) {
        return false;
    }
    std::ifstream input(path.c_str());
    if (!input) {
        if (error) *error = "cannot open config file: " + path;
        return false;
    }

    std::string line;
    unsigned int lineNumber = 0;
    while (std::getline(input, line)) {
        lineNumber += 1;
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            if (error) {
                std::ostringstream out;
                out << "invalid config line " << lineNumber << ": expected key=value";
                *error = out.str();
            }
            return false;
        }
        const std::string key = Trim(line.substr(0, equals));
        const std::string value = Trim(line.substr(equals + 1));
        if (!AddConfigOption(key, value, args, error)) {
            return false;
        }
    }
    return true;
}

bool BuildMergedArgsWithConfig(int argc, char **argv, std::vector<std::string>& merged, std::string *error)
{
    merged.clear();
    merged.push_back(argv[0]);

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--config") {
            if (i + 1 >= argc) {
                if (error) *error = "missing --config value";
                return false;
            }
            if (!AppendConfigFileArgs(argv[++i], merged, error)) {
                return false;
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--config") {
            ++i;
            continue;
        }
        merged.push_back(arg);
    }
    return true;
}

bool ParseUnsigned(const char *value, unsigned int min, unsigned int max, unsigned int& out)
{
    if (!value || !*value) {
        return false;
    }
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (*end != '\0' || parsed < min || parsed > max) {
        return false;
    }
    out = static_cast<unsigned int>(parsed);
    return true;
}

bool ParseArgs(int argc, char **argv, ServerConfig& config, CaptureBackend& captureBackend, InputBackend& inputBackend, std::string& rawFramebufferFile, std::string& passwordFile, bool& validateOnly, bool& printConfig, bool& smokeTest, bool& smokeUpdateTest, bool& smokeMultiUpdateTest, bool& smokeRawFileUpdateTest, bool& smokeX11UpdateTest, bool& smokeX11AvailabilityTest, bool& smokePipeWireAvailabilityTest, bool& smokeXTestAvailabilityTest, bool& smokeXTestInputTest, bool& allowInputInjection, bool& serveUpdates, bool& serveForever, unsigned int& maxUpdates, std::string& pidFile, std::string& statusFile, std::string& logFile)
{
    validateOnly = false;
    printConfig = false;
    smokeTest = false;
    smokeUpdateTest = false;
    smokeMultiUpdateTest = false;
    smokeRawFileUpdateTest = false;
    smokeX11UpdateTest = false;
    smokeX11AvailabilityTest = false;
    smokePipeWireAvailabilityTest = false;
    smokeXTestAvailabilityTest = false;
    smokeXTestInputTest = false;
    allowInputInjection = false;
    serveUpdates = false;
    serveForever = false;
    maxUpdates = 3;
    pidFile.clear();
    statusFile.clear();
    logFile.clear();
    captureBackend = CaptureBackend::Auto;
    inputBackend = InputBackend::Auto;
    rawFramebufferFile.clear();
    passwordFile.clear();
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (arg == "--validate-config") {
            validateOnly = true;
        } else if (arg == "--print-config") {
            printConfig = true;
        } else if (arg == "--smoke-test") {
            smokeTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
            config.SetAllowNoAuth(true);
        } else if (arg == "--smoke-update-test") {
            smokeUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
            config.SetAllowNoAuth(true);
        } else if (arg == "--smoke-multi-update-test") {
            smokeMultiUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
            config.SetAllowNoAuth(true);
        } else if (arg == "--smoke-raw-file-update-test") {
            smokeRawFileUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
            config.SetAllowNoAuth(true);
        } else if (arg == "--smoke-x11-update-test") {
            smokeX11UpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
            config.SetAllowNoAuth(true);
        } else if (arg == "--smoke-x11-availability-test") {
            smokeX11AvailabilityTest = true;
        } else if (arg == "--smoke-pipewire-availability-test") {
            smokePipeWireAvailabilityTest = true;
        } else if (arg == "--smoke-xtest-availability-test") {
            smokeXTestAvailabilityTest = true;
        } else if (arg == "--smoke-xtest-input-test") {
            smokeXTestInputTest = true;
        } else if (arg == "--allow-input-injection") {
            allowInputInjection = true;
        } else if (arg == "--allow-no-auth") {
            config.SetAllowNoAuth(true);
        } else if (arg == "--allow-public-no-auth") {
            config.SetAllowPublicNoAuth(true);
        } else if (arg == "--allow-unencrypted-public") {
            config.SetAllowUnencryptedPublic(true);
        } else if (arg == "--auth" && i + 1 < argc) {
            ServerAuthMode mode = ServerAuthMode::NoAuth;
            if (!ParseServerAuthMode(argv[++i], mode)) {
                std::cerr << "invalid --auth\n";
                return false;
            }
            config.SetAuthMode(mode);
        } else if (arg == "--password-file" && i + 1 < argc) {
            passwordFile = argv[++i];
        } else if (arg == "--serve-updates") {
            serveUpdates = true;
        } else if (arg == "--serve-forever") {
            serveForever = true;
        } else if (arg == "--pid-file" && i + 1 < argc) {
            pidFile = argv[++i];
        } else if (arg == "--status-file" && i + 1 < argc) {
            statusFile = argv[++i];
        } else if (arg == "--log-file" && i + 1 < argc) {
            logFile = argv[++i];
        } else if (arg == "--max-updates" && i + 1 < argc) {
            if (!ParseUnsigned(argv[++i], 1, 1024, maxUpdates)) {
                std::cerr << "invalid --max-updates\n";
                return false;
            }
        } else if (arg == "--bind-address" && i + 1 < argc) {
            config.SetBindAddress(argv[++i]);
        } else if (arg == "--name" && i + 1 < argc) {
            config.SetDesktopName(argv[++i]);
        } else if (arg == "--capture-backend" && i + 1 < argc) {
            if (!ParseCaptureBackendName(argv[++i], captureBackend)) {
                std::cerr << "invalid --capture-backend\n";
                return false;
            }
        } else if (arg == "--raw-framebuffer-file" && i + 1 < argc) {
            rawFramebufferFile = argv[++i];
        } else if (arg == "--input-backend" && i + 1 < argc) {
            if (!ParseInputBackendName(argv[++i], inputBackend)) {
                std::cerr << "invalid --input-backend\n";
                return false;
            }
        } else if (arg == "--pattern" && i + 1 < argc) {
            FramebufferPattern pattern = FramebufferPattern::Solid;
            if (!ParseFramebufferPattern(argv[++i], pattern)) {
                std::cerr << "invalid --pattern\n";
                return false;
            }
            config.SetPattern(pattern);
        } else if (arg == "--fill-byte" && i + 1 < argc) {
            unsigned int fillByte = 0;
            if (!ParseUnsigned(argv[++i], 0, 255, fillByte)) {
                std::cerr << "invalid --fill-byte\n";
                return false;
            }
            config.SetFillByte(static_cast<unsigned char>(fillByte));
        } else if (arg == "--port" && i + 1 < argc) {
            unsigned int port = 0;
            if (!ParseUnsigned(argv[++i], 0, 65535, port)) {
                std::cerr << "invalid --port\n";
                return false;
            }
            config.SetPort(static_cast<unsigned short>(port));
        } else if (arg == "--width" && i + 1 < argc) {
            unsigned int width = 0;
            if (!ParseUnsigned(argv[++i], 1, 16384, width)) {
                std::cerr << "invalid --width\n";
                return false;
            }
            config.SetSize(width, config.Height());
        } else if (arg == "--height" && i + 1 < argc) {
            unsigned int height = 0;
            if (!ParseUnsigned(argv[++i], 1, 16384, height)) {
                std::cerr << "invalid --height\n";
                return false;
            }
            config.SetSize(config.Width(), height);
        } else {
            std::cerr << "unknown or incomplete option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

void EncryptVncAuthChallengeForClient(std::vector<CARD8>& challenge, const std::string& password)
{
    unsigned char key[8] = {};
    for (std::size_t i = 0; i < sizeof(key) && i < password.size(); ++i) {
        key[i] = static_cast<unsigned char>(password[i]);
    }
    deskey(key, EN0);
    for (std::size_t i = 0; i + 8 <= challenge.size(); i += 8) {
        des(challenge.data() + i, challenge.data() + i);
    }
}

bool RunMemoryServerClientHandshake(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) {
        return false;
    }
    const std::string clientVersion = uvnc::winvnc::portable::ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) {
        return false;
    }
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) {
        return false;
    }
    const CARD8 selected = config.AuthMode() == ServerAuthMode::VncPassword ? rfbVncAuth : rfbNoAuth;
    if (security[0] != 1 || security[1] != selected) {
        return false;
    }
    if (!client.WriteAll(&selected, sizeof(selected))) {
        return false;
    }
    if (config.AuthMode() == ServerAuthMode::VncPassword) {
        std::vector<CARD8> challenge(16);
        if (!client.ReadExact(challenge.data(), challenge.size())) {
            return false;
        }
        EncryptVncAuthChallengeForClient(challenge, config.VncPassword());
        if (!client.WriteAll(challenge.data(), challenge.size())) {
            return false;
        }
    }
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || Swap32IfLE(auth) != rfbVncAuthOK) {
        return false;
    }
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) {
        return false;
    }
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) {
        return false;
    }
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    return client.ReadExact(&name[0], name.size()) &&
           Swap16IfLE(serverInit.framebufferWidth) == config.Width() &&
           Swap16IfLE(serverInit.framebufferHeight) == config.Height() &&
           name == config.DesktopName();
}

bool RunSmokeTest(const ServerConfig& config)
{
    MemoryServer server;
    if (!server.Start(config)) {
        std::cerr << "failed to start memory server smoke test\n";
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOne();
    });

    TcpSocket client;
    const bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                          RunMemoryServerClientHandshake(client, config);

    worker.join();
    server.Stop();
    return clientOk && serverOk;
}

bool RunSmokeUpdateTest(const ServerConfig& config)
{
    MemoryServer server;
    if (!server.Start(config)) {
        std::cerr << "failed to start memory server update smoke test\n";
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdate();
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                    RunMemoryServerClientHandshake(client, config);
    if (clientOk) {
        FramebufferUpdateRequest request;
        request.incremental = false;
        request.x = 0;
        request.y = 0;
        request.width = config.Width();
        request.height = config.Height();
        const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
        clientOk = client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);

        rfbFramebufferUpdateMsg update;
        clientOk = clientOk && client.ReadExact(&update, sz_rfbFramebufferUpdateMsg);
        clientOk = clientOk && Swap16IfLE(update.nRects) == 1;
        rfbFramebufferUpdateRectHeader header;
        clientOk = clientOk && client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader);
        clientOk = clientOk && Swap16IfLE(header.r.w) == config.Width() &&
                   Swap16IfLE(header.r.h) == config.Height() &&
                   Swap32IfLE(header.encoding) == rfbEncodingRaw;
        std::string pixels(config.Width() * config.Height() * (config.PixelFormat().bitsPerPixel / 8), '\0');
        clientOk = clientOk && client.ReadExact(&pixels[0], pixels.size());
        clientOk = clientOk && std::all_of(pixels.begin(), pixels.end(), [&](char byte) {
            return static_cast<unsigned char>(byte) == config.FillByte();
        });
    }

    worker.join();
    server.Stop();
    return clientOk && serverOk;
}

bool RunSmokeMultiUpdateTest(const ServerConfig& config, unsigned int maxUpdates)
{
    MemoryServer server;
    if (!server.Start(config)) {
        std::cerr << "failed to start memory server multi-update smoke test\n";
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdates(maxUpdates);
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                    RunMemoryServerClientHandshake(client, config);
    for (unsigned int i = 0; clientOk && i < maxUpdates; ++i) {
        FramebufferUpdateRequest request;
        request.incremental = i != 0;
        request.x = 0;
        request.y = 0;
        request.width = config.Width();
        request.height = config.Height();
        const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
        clientOk = client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);

        rfbFramebufferUpdateMsg update;
        clientOk = clientOk && client.ReadExact(&update, sz_rfbFramebufferUpdateMsg);
        if (i == 0) {
            clientOk = clientOk && Swap16IfLE(update.nRects) == 1;
            rfbFramebufferUpdateRectHeader header;
            clientOk = clientOk && client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader);
            clientOk = clientOk && Swap16IfLE(header.r.w) == config.Width() &&
                       Swap16IfLE(header.r.h) == config.Height() &&
                       Swap32IfLE(header.encoding) == rfbEncodingRaw;
            std::string pixels(config.Width() * config.Height() * (config.PixelFormat().bitsPerPixel / 8), '\0');
            clientOk = clientOk && client.ReadExact(&pixels[0], pixels.size());
            clientOk = clientOk && std::all_of(pixels.begin(), pixels.end(), [&](char byte) {
                return static_cast<unsigned char>(byte) == config.FillByte();
            });
        } else {
            clientOk = clientOk && Swap16IfLE(update.nRects) == 0;
        }
    }

    worker.join();
    server.Stop();
    return clientOk && serverOk;
}

void PrintResolvedConfig(const ServerConfig& config, CaptureBackend requestedBackend, CaptureBackend resolvedBackend, InputBackend requestedInputBackend, InputBackend resolvedInputBackend, bool serveUpdates, bool serveForever, unsigned int maxUpdates, const std::string& pidFile, const std::string& statusFile, const std::string& logFile)
{
    std::cout << "bind_address=" << config.BindAddress() << "\n"
              << "port=" << config.Port() << "\n"
              << "width=" << config.Width() << "\n"
              << "height=" << config.Height() << "\n"
              << "name=" << config.DesktopName() << "\n"
              << "fill_byte=" << static_cast<unsigned int>(config.FillByte()) << "\n"
              << "pattern=" << FramebufferPatternName(config.Pattern()) << "\n"
              << "capture_backend=" << CaptureBackendName(requestedBackend) << "\n"
              << "resolved_capture_backend=" << CaptureBackendName(resolvedBackend) << "\n"
              << "input_backend=" << InputBackendName(requestedInputBackend) << "\n"
              << "resolved_input_backend=" << InputBackendName(resolvedInputBackend) << "\n"
              << "auth=" << ServerAuthModeName(config.AuthMode()) << "\n"
              << "allow_no_auth=" << (config.AllowNoAuth() ? "yes" : "no") << "\n"
              << "allow_public_no_auth=" << (config.AllowPublicNoAuth() ? "yes" : "no") << "\n"
              << "allow_unencrypted_public=" << (config.AllowUnencryptedPublic() ? "yes" : "no") << "\n"
              << "serve_updates=" << (serveUpdates ? "yes" : "no") << "\n"
              << "serve_forever=" << (serveForever ? "yes" : "no") << "\n"
              << "max_updates=" << maxUpdates << "\n"
              << "pid_file=" << pidFile << "\n"
              << "status_file=" << statusFile << "\n"
              << "log_file=" << logFile << "\n";
}

ServerConfig ConfigForCapturedFramebuffer(const ServerConfig& base, const Framebuffer& framebuffer)
{
    ServerConfig config = base;
    config.SetSize(framebuffer.Width(), framebuffer.Height());
    config.SetPixelFormat(framebuffer.Format());
    return config;
}

bool RunSmokeRawFileUpdateTest(const ServerConfig& config)
{
    const std::string rawPath = "/tmp/uvnc-winvnc-raw-file-update-smoke.bin";
    const std::size_t size = config.Width() * config.Height() * (config.PixelFormat().bitsPerPixel / 8);
    {
        std::ofstream out(rawPath.c_str(), std::ios::binary);
        std::string pixels(size, static_cast<char>(config.FillByte()));
        out.write(pixels.data(), static_cast<std::streamsize>(pixels.size()));
    }

    Framebuffer framebuffer;
    std::string error;
    if (!LoadRawFramebufferFile(rawPath, config.Width(), config.Height(), config.PixelFormat(), framebuffer, &error)) {
        std::cerr << "failed to load raw framebuffer smoke file: " << error << "\n";
        return false;
    }

    MemoryServer server;
    if (!server.StartWithFramebuffer(config, framebuffer)) {
        std::cerr << "failed to start raw framebuffer file smoke server\n";
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdate();
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                    RunMemoryServerClientHandshake(client, config);
    if (clientOk) {
        FramebufferUpdateRequest request;
        request.incremental = false;
        request.x = 0;
        request.y = 0;
        request.width = config.Width();
        request.height = config.Height();
        const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
        clientOk = client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);

        rfbFramebufferUpdateMsg update;
        clientOk = clientOk && client.ReadExact(&update, sz_rfbFramebufferUpdateMsg);
        clientOk = clientOk && Swap16IfLE(update.nRects) == 1;
        rfbFramebufferUpdateRectHeader header;
        clientOk = clientOk && client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader);
        clientOk = clientOk && Swap16IfLE(header.r.w) == config.Width() &&
                   Swap16IfLE(header.r.h) == config.Height() &&
                   Swap32IfLE(header.encoding) == rfbEncodingRaw;
        std::string pixels(size, '\0');
        clientOk = clientOk && client.ReadExact(&pixels[0], pixels.size());
        clientOk = clientOk && std::all_of(pixels.begin(), pixels.end(), [&](char byte) {
            return static_cast<unsigned char>(byte) == config.FillByte();
        });
    }

    worker.join();
    server.Stop();
    return clientOk && serverOk;
}

bool RunSmokeX11UpdateTest(const ServerConfig& config)
{
    X11DesktopSource source;
    Framebuffer framebuffer;
    rfb::Region2D changed;
    if (!source.Snapshot(framebuffer, changed)) {
        std::cerr << "X11 smoke update test skipped: " << X11DesktopSource::UnavailableReason() << "\n";
        return false;
    }

    const ServerConfig capturedConfig = ConfigForCapturedFramebuffer(config, framebuffer);

    MemoryServer server;
    if (!server.StartWithFramebuffer(capturedConfig, framebuffer)) {
        std::cerr << "failed to start X11 snapshot memory server\n";
        return false;
    }

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdate();
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) &&
                    RunMemoryServerClientHandshake(client, capturedConfig);
    if (clientOk) {
        FramebufferUpdateRequest request;
        request.incremental = false;
        request.x = 0;
        request.y = 0;
        request.width = framebuffer.Width();
        request.height = framebuffer.Height();
        const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
        clientOk = client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);

        rfbFramebufferUpdateMsg update;
        clientOk = clientOk && client.ReadExact(&update, sz_rfbFramebufferUpdateMsg);
        clientOk = clientOk && Swap16IfLE(update.nRects) == 1;
        rfbFramebufferUpdateRectHeader header;
        clientOk = clientOk && client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader);
        const unsigned int width = Swap16IfLE(header.r.w);
        const unsigned int height = Swap16IfLE(header.r.h);
        clientOk = clientOk && width == framebuffer.Width() && height == framebuffer.Height();
        std::string pixels(width * height * framebuffer.BytesPerPixel(), '\0');
        clientOk = clientOk && client.ReadExact(&pixels[0], pixels.size());
    }

    worker.join();
    server.Stop();
    return clientOk && serverOk;
}

int RunSmokeX11AvailabilityTest()
{
    const bool buildAvailable = X11DesktopSource::IsBuildAvailable();
    const bool runtimeAvailable = X11DesktopSource::IsAvailable();
    std::cout << "x11-build-available=" << (buildAvailable ? "yes" : "no") << "\n";
    std::cout << "x11-runtime-available=" << (runtimeAvailable ? "yes" : "no") << "\n";
    std::cout << "x11-xshm-build-available=" << (X11DesktopSource::IsXShmBuildAvailable() ? "yes" : "no") << "\n";
    std::cout << "x11-xshm-runtime-available=" << (X11DesktopSource::IsXShmRuntimeAvailable() ? "yes" : "no") << "\n";
    std::cout << "x11-unavailable-reason=" << (runtimeAvailable ? "" : X11DesktopSource::UnavailableReason()) << "\n";
    return 0;
}

int RunSmokePipeWireAvailabilityTest()
{
    std::string reason;
    const PipeWirePortalRuntimeState state = PipeWirePortalCaptureBackend::RuntimeState(&reason);
    std::cout << "pipewire-portal-runtime=" << PipeWirePortalCaptureBackend::RuntimeStateName(state) << "\n";
    std::cout << "pipewire-portal-reason=" << reason << "\n";
    return 0;
}

int RunSmokeXTestAvailabilityTest()
{
    const bool buildAvailable = XTestInputBackend::IsBuildAvailable();
    const bool runtimeAvailable = XTestInputBackend::IsAvailable();
    std::cout << "xtest-build-available=" << (buildAvailable ? "yes" : "no") << "\n";
    std::cout << "xtest-runtime-available=" << (runtimeAvailable ? "yes" : "no") << "\n";
    std::cout << "xtest-unavailable-reason=" << (runtimeAvailable ? "" : XTestInputBackend::UnavailableReason()) << "\n";
    return 0;
}

int RunSmokeXTestInputTest(bool allowInputInjection)
{
    if (!allowInputInjection) {
        std::cout << "Skipping live XTest input smoke because --allow-input-injection was not provided.\n";
        return 0;
    }
    if (!XTestInputBackend::IsAvailable()) {
        std::cout << "Skipping live XTest input smoke: " << XTestInputBackend::UnavailableReason() << "\n";
        return 0;
    }

    XTestInputBackend input;
    std::string error;
    if (!input.InjectKeySym(0xffe3, true, &error) || !input.InjectKeySym(0xffe3, false, &error)) { // XK_Control_L press/release, intentionally low-impact.
        std::cerr << "XTest key injection smoke failed: " << error << "\n";
        return 1;
    }
    if (!input.InjectPointerRelative(24, 24, &error) ||
        !input.InjectButton(1, true, &error) ||
        !input.InjectButton(1, false, &error) ||
        !input.InjectPointerRelative(-24, -24, &error)) {
        std::cerr << "XTest pointer injection smoke failed: " << error << "\n";
        return 1;
    }
    std::cout << "Injected XTest Control_L press/release, pointer motion, and button click.\n";
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    ServerConfig config;
    CaptureBackend captureBackend = CaptureBackend::Auto;
    CaptureBackend resolvedCaptureBackend = CaptureBackend::Memory;
    InputBackend inputBackend = InputBackend::Auto;
    InputBackend resolvedInputBackend = InputBackend::None;
    std::string rawFramebufferFile;
    std::string passwordFile;
    bool validateOnly = false;
    bool printConfig = false;
    bool smokeTest = false;
    bool smokeUpdateTest = false;
    bool smokeMultiUpdateTest = false;
    bool smokeRawFileUpdateTest = false;
    bool smokeX11UpdateTest = false;
    bool smokeX11AvailabilityTest = false;
    bool smokePipeWireAvailabilityTest = false;
    bool smokeXTestAvailabilityTest = false;
    bool smokeXTestInputTest = false;
    bool allowInputInjection = false;
    bool serveUpdates = false;
    bool serveForever = false;
    unsigned int maxUpdates = 3;
    std::string pidFile;
    std::string statusFile;
    std::string logFile;
    std::string error;
    std::vector<std::string> mergedArgs;
    if (!BuildMergedArgsWithConfig(argc, argv, mergedArgs, &error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    std::vector<char *> mergedArgv;
    for (std::size_t i = 0; i < mergedArgs.size(); ++i) {
        mergedArgv.push_back(&mergedArgs[i][0]);
    }

    if (!ParseArgs(static_cast<int>(mergedArgv.size()), mergedArgv.data(), config, captureBackend, inputBackend, rawFramebufferFile, passwordFile, validateOnly, printConfig, smokeTest, smokeUpdateTest, smokeMultiUpdateTest, smokeRawFileUpdateTest, smokeX11UpdateTest, smokeX11AvailabilityTest, smokePipeWireAvailabilityTest, smokeXTestAvailabilityTest, smokeXTestInputTest, allowInputInjection, serveUpdates, serveForever, maxUpdates, pidFile, statusFile, logFile)) {
        return 2;
    }

    HardenRuntimeFileCreationUmask();

    if (!passwordFile.empty()) {
        std::string password;
        if (!ReadPasswordFile(passwordFile, password, &error)) {
            std::cerr << "invalid config: " << error << "\n";
            return 2;
        }
        config.SetAuthMode(ServerAuthMode::VncPassword);
        config.SetVncPassword(password);
    }

    InstallStopSignalHandlers();
    if (!config.Validate(&error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    if (smokeX11AvailabilityTest) {
        return RunSmokeX11AvailabilityTest();
    }
    if (smokePipeWireAvailabilityTest) {
        return RunSmokePipeWireAvailabilityTest();
    }
    if (smokeXTestAvailabilityTest) {
        return RunSmokeXTestAvailabilityTest();
    }
    if (smokeXTestInputTest) {
        return RunSmokeXTestInputTest(allowInputInjection);
    }
    if (!EnforceLinuxServerSecurityPolicy(config, &error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    if (validateOnly || printConfig) {
        PrintSecurityWarnings(config);
    }
    if (!ResolveCaptureBackend(captureBackend, !rawFramebufferFile.empty(), resolvedCaptureBackend, &error)) {
        std::cerr << "invalid capture backend: " << error << "\n";
        return 2;
    }
    if (!ResolveInputBackend(inputBackend, resolvedInputBackend, &error)) {
        std::cerr << "invalid input backend: " << error << "\n";
        return 2;
    }
    if (!ValidateRuntimeFilePaths(pidFile, statusFile, logFile, &error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    if (validateOnly) {
        return 0;
    }
    if (printConfig) {
        PrintResolvedConfig(config, captureBackend, resolvedCaptureBackend, inputBackend, resolvedInputBackend, serveUpdates, serveForever, maxUpdates, pidFile, statusFile, logFile);
        return 0;
    }
    if (smokeTest) {
        return RunSmokeTest(config) ? 0 : 1;
    }
    if (smokeUpdateTest) {
        return RunSmokeUpdateTest(config) ? 0 : 1;
    }
    if (smokeMultiUpdateTest) {
        return RunSmokeMultiUpdateTest(config, maxUpdates) ? 0 : 1;
    }
    if (smokeRawFileUpdateTest) {
        return RunSmokeRawFileUpdateTest(config) ? 0 : 1;
    }
    if (smokeX11UpdateTest) {
        return RunSmokeX11UpdateTest(config) ? 0 : 1;
    }

    XTestRfbInputSink xtestInputSink;
    RfbInputSink *inputSink = resolvedInputBackend == InputBackend::XTest ? &xtestInputSink : nullptr;

    std::ofstream logStream;
    ScopedStreamBufferRedirect coutRedirect;
    ScopedStreamBufferRedirect cerrRedirect;
    if (!logFile.empty()) {
        logStream.open(logFile.c_str(), std::ios::app);
        if (!logStream) {
            std::cerr << "invalid config: cannot open log file: " << logFile << "\n";
            return 2;
        }
        coutRedirect.Redirect(std::cout, logStream.rdbuf());
        cerrRedirect.Redirect(std::cerr, logStream.rdbuf());
    }

    PrintSecurityWarnings(config);

    if (!WriteTextFile(statusFile, "starting\n")) {
        std::cerr << "failed to write status file: " << statusFile << "\n";
        return 1;
    }

    MemoryServer server;
    X11DesktopSource x11Source;
    DesktopSource *liveSource = nullptr;
    if (resolvedCaptureBackend == CaptureBackend::RawFile) {
        Framebuffer framebuffer;
        std::string loadError;
        if (!LoadRawFramebufferFile(rawFramebufferFile, config.Width(), config.Height(), config.PixelFormat(), framebuffer, &loadError) ||
            !server.StartWithFramebuffer(config, framebuffer)) {
            std::cerr << "failed to start raw framebuffer file server: " << loadError << "\n";
            return 1;
        }
    } else if (resolvedCaptureBackend == CaptureBackend::X11) {
        Framebuffer framebuffer;
        rfb::Region2D changed;
        if (!x11Source.Snapshot(framebuffer, changed)) {
            std::cerr << "failed to capture X11 framebuffer: " << x11Source.LastError() << "\n";
            return 1;
        }
        const ServerConfig capturedConfig = ConfigForCapturedFramebuffer(config, framebuffer);
        if (!server.StartWithFramebuffer(capturedConfig, framebuffer)) {
            std::cerr << "failed to start X11 framebuffer server\n";
            return 1;
        }
        liveSource = &x11Source;
    } else if (!server.Start(config)) {
        std::cerr << "failed to start memory server\n";
        return 1;
    }
    if (!WriteTextFile(pidFile, std::to_string(static_cast<long long>(getpid())) + "\n")) {
        std::cerr << "failed to write pid file: " << pidFile << "\n";
        server.Stop();
        WriteTextFile(statusFile, "failed\n");
        return 1;
    }

    const std::string listeningStatus = std::string("listening ") + config.BindAddress() + ":" + std::to_string(server.Port()) + "\n";
    WriteTextFile(statusFile, listeningStatus);
    std::cout << "listening on " << config.BindAddress() << ":" << server.Port() << "\n" << std::flush;

    bool served = false;
    if (serveForever) {
        served = true;
        while (!StopRequested()) {
            bool accepted = false;
            const bool ok = liveSource ?
                server.TryServeOneUpdatesFromSource(*liveSource, maxUpdates, inputSink, 128, 250, accepted) :
                server.TryServeOneUpdates(maxUpdates, inputSink, 250, accepted);
            if (!ok) {
                served = false;
                break;
            }
        }
    } else {
        served = serveUpdates && liveSource ? server.ServeOneUpdatesFromSource(*liveSource, maxUpdates, inputSink) :
            (serveUpdates ? server.ServeOneUpdates(maxUpdates, inputSink) : server.ServeOne());
    }

    WriteTextFile(statusFile, StopRequested() ? "stopping\n" : (served ? "completed\n" : "failed\n"));
    server.Stop();
    RemoveFileIfSet(pidFile);
    WriteTextFile(statusFile, served ? "stopped\n" : "failed\n");
    return served ? 0 : 1;
}
