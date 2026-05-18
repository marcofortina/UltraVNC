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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using uvnc::winvnc::portable::ServerConfig;
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
using uvnc::winvnc::portable::Framebuffer;
using uvnc::winvnc::portable::MemoryServer;
using uvnc::winvnc::portable::FramebufferUpdateRequest;
using uvnc::winvnc::portable::TcpSocket;
using uvnc::winvnc::portable::FramebufferPattern;
using uvnc::winvnc::portable::FramebufferPatternName;
using uvnc::winvnc::portable::ParseFramebufferPattern;

namespace {

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
              << "  --validate-config       Validate options and exit\n"
              << "  --print-config          Print resolved configuration and exit\n"
              << "  --smoke-test            Start on loopback, complete one RFB handshake, and exit\n"
              << "  --smoke-update-test     Start on loopback, request one raw framebuffer update, and exit\n"
              << "  --smoke-multi-update-test Start on loopback, request multiple raw framebuffer updates, and exit\n"
              << "  --smoke-raw-file-update-test Start on loopback using a raw framebuffer file, request one update, and exit\n"
              << "  --smoke-x11-update-test Start on loopback using an X11 snapshot, request one update, and exit\n"
              << "  --smoke-pipewire-availability-test Print PipeWire/XDG portal runtime availability and exit\n"
              << "  --smoke-xtest-availability-test Print XTest input runtime availability and exit\n"
              << "  --max-updates <count>  Number of updates for multi-update smoke/serve mode, default 3\n"
              << "  --serve-updates        Serve one client through --max-updates framebuffer updates\n"
              << "  --help                  Show this help\n";
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

bool ParseArgs(int argc, char **argv, ServerConfig& config, CaptureBackend& captureBackend, InputBackend& inputBackend, std::string& rawFramebufferFile, bool& validateOnly, bool& printConfig, bool& smokeTest, bool& smokeUpdateTest, bool& smokeMultiUpdateTest, bool& smokeRawFileUpdateTest, bool& smokeX11UpdateTest, bool& smokePipeWireAvailabilityTest, bool& smokeXTestAvailabilityTest, bool& serveUpdates, unsigned int& maxUpdates)
{
    validateOnly = false;
    printConfig = false;
    smokeTest = false;
    smokeUpdateTest = false;
    smokeMultiUpdateTest = false;
    smokeRawFileUpdateTest = false;
    smokeX11UpdateTest = false;
    smokePipeWireAvailabilityTest = false;
    smokeXTestAvailabilityTest = false;
    serveUpdates = false;
    maxUpdates = 3;
    captureBackend = CaptureBackend::Auto;
    inputBackend = InputBackend::Auto;
    rawFramebufferFile.clear();
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
        } else if (arg == "--smoke-update-test") {
            smokeUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
        } else if (arg == "--smoke-multi-update-test") {
            smokeMultiUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
        } else if (arg == "--smoke-raw-file-update-test") {
            smokeRawFileUpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
        } else if (arg == "--smoke-x11-update-test") {
            smokeX11UpdateTest = true;
            config.SetBindAddress("127.0.0.1");
            config.SetPort(0);
        } else if (arg == "--smoke-pipewire-availability-test") {
            smokePipeWireAvailabilityTest = true;
        } else if (arg == "--smoke-xtest-availability-test") {
            smokeXTestAvailabilityTest = true;
        } else if (arg == "--serve-updates") {
            serveUpdates = true;
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
    CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) {
        return false;
    }
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != 0) {
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

void PrintResolvedConfig(const ServerConfig& config, CaptureBackend requestedBackend, CaptureBackend resolvedBackend, InputBackend requestedInputBackend, InputBackend resolvedInputBackend, unsigned int maxUpdates)
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
              << "max_updates=" << maxUpdates << "\n";
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

    MemoryServer server;
    if (!server.StartWithFramebuffer(config, framebuffer)) {
        std::cerr << "failed to start X11 snapshot memory server\n";
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
    std::cout << "xtest-build-available=" << (XTestInputBackend::IsBuildAvailable() ? "yes" : "no") << "\n";
    std::cout << "xtest-runtime-available=" << (XTestInputBackend::IsAvailable() ? "yes" : "no") << "\n";
    std::cout << "xtest-unavailable-reason=" << XTestInputBackend::UnavailableReason() << "\n";
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
    bool validateOnly = false;
    bool printConfig = false;
    bool smokeTest = false;
    bool smokeUpdateTest = false;
    bool smokeMultiUpdateTest = false;
    bool smokeRawFileUpdateTest = false;
    bool smokeX11UpdateTest = false;
    bool smokePipeWireAvailabilityTest = false;
    bool smokeXTestAvailabilityTest = false;
    bool serveUpdates = false;
    unsigned int maxUpdates = 3;
    if (!ParseArgs(argc, argv, config, captureBackend, inputBackend, rawFramebufferFile, validateOnly, printConfig, smokeTest, smokeUpdateTest, smokeMultiUpdateTest, smokeRawFileUpdateTest, smokeX11UpdateTest, smokePipeWireAvailabilityTest, smokeXTestAvailabilityTest, serveUpdates, maxUpdates)) {
        return 2;
    }
    std::string error;
    if (!config.Validate(&error)) {
        std::cerr << "invalid config: " << error << "\n";
        return 2;
    }
    if (smokePipeWireAvailabilityTest) {
        return RunSmokePipeWireAvailabilityTest();
    }
    if (smokeXTestAvailabilityTest) {
        return RunSmokeXTestAvailabilityTest();
    }
    if (!ResolveCaptureBackend(captureBackend, !rawFramebufferFile.empty(), resolvedCaptureBackend, &error)) {
        std::cerr << "invalid capture backend: " << error << "\n";
        return 2;
    }
    if (!ResolveInputBackend(inputBackend, resolvedInputBackend, &error)) {
        std::cerr << "invalid input backend: " << error << "\n";
        return 2;
    }
    if (validateOnly) {
        return 0;
    }
    if (printConfig) {
        PrintResolvedConfig(config, captureBackend, resolvedCaptureBackend, inputBackend, resolvedInputBackend, maxUpdates);
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

    MemoryServer server;
    if (resolvedCaptureBackend == CaptureBackend::RawFile) {
        Framebuffer framebuffer;
        std::string loadError;
        if (!LoadRawFramebufferFile(rawFramebufferFile, config.Width(), config.Height(), config.PixelFormat(), framebuffer, &loadError) ||
            !server.StartWithFramebuffer(config, framebuffer)) {
            std::cerr << "failed to start raw framebuffer file server: " << loadError << "\n";
            return 1;
        }
    } else if (resolvedCaptureBackend == CaptureBackend::X11) {
        X11DesktopSource source;
        Framebuffer framebuffer;
        rfb::Region2D changed;
        if (!source.Snapshot(framebuffer, changed) || !server.StartWithFramebuffer(config, framebuffer)) {
            std::cerr << "failed to start X11 framebuffer server\n";
            return 1;
        }
    } else if (!server.Start(config)) {
        std::cerr << "failed to start memory server\n";
        return 1;
    }
    std::cout << "listening on " << config.BindAddress() << ":" << server.Port() << "\n";
    const bool served = serveUpdates ? server.ServeOneUpdates(maxUpdates) : server.ServeOne();
    server.Stop();
    return served ? 0 : 1;
}
