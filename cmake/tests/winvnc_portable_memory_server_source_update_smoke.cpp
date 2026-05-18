// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDesktopSource.h"
#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

class CountingDesktopSource : public DesktopSource {
public:
    explicit CountingDesktopSource(const rfbPixelFormat& format)
        : format_(format), fill_(0x30)
    {
    }

    rfb::Rect Size() const override { return rfb::Rect(0, 0, 4, 4); }
    rfbPixelFormat Format() const override { return format_; }

    bool Snapshot(Framebuffer& destination, rfb::Region2D& changed) override
    {
        if (!destination.Reset(4, 4, format_)) {
            return false;
        }
        destination.Fill(fill_++);
        changed.reset(destination.Bounds());
        return true;
    }

private:
    rfbPixelFormat format_;
    unsigned char fill_;
};

bool ClientHandshake(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) return false;
    const std::string version38 = ProtocolVersion38();
    if (!client.WriteAll(version38.data(), version38.size())) return false;
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) return false;
    CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) return false;
    CARD32 auth = 1;
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != 0) return false;
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    std::string name(Swap32IfLE(serverInit.nameLength), '\0');
    return client.ReadExact(&name[0], name.size()) &&
           Swap16IfLE(serverInit.framebufferWidth) == config.Width() &&
           Swap16IfLE(serverInit.framebufferHeight) == config.Height();
}

bool ReadRawUpdate(TcpSocket& client, unsigned char& firstPixel)
{
    rfbFramebufferUpdateMsg update;
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) {
        return false;
    }
    rfbFramebufferUpdateRectHeader rect;
    if (!client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader) ||
        Swap16IfLE(rect.r.w) != 4 ||
        Swap16IfLE(rect.r.h) != 4 ||
        Swap32IfLE(rect.encoding) != rfbEncodingRaw) {
        return false;
    }
    std::string pixels(4 * 4 * 4, '\0');
    if (!client.ReadExact(&pixels[0], pixels.size())) {
        return false;
    }
    firstPixel = static_cast<unsigned char>(pixels[0]);
    return true;
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(4, 4);

    CountingDesktopSource source(config.PixelFormat());
    Framebuffer initial;
    rfb::Region2D changed;
    assert(source.Snapshot(initial, changed));

    MemoryServer server;
    assert(server.StartWithFramebuffer(config, initial));

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdatesFromSource(source, 2);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(ClientHandshake(client, config));

    FramebufferUpdateRequest full{false, 0, 0, 4, 4};
    const rfbFramebufferUpdateRequestMsg fullWire = EncodeFramebufferUpdateRequest(full);
    assert(client.WriteAll(&fullWire, sz_rfbFramebufferUpdateRequestMsg));

    unsigned char firstFull = 0;
    assert(ReadRawUpdate(client, firstFull));

    FramebufferUpdateRequest incremental{true, 0, 0, 4, 4};
    const rfbFramebufferUpdateRequestMsg incrementalWire = EncodeFramebufferUpdateRequest(incremental);
    assert(client.WriteAll(&incrementalWire, sz_rfbFramebufferUpdateRequestMsg));

    unsigned char firstIncremental = 0;
    assert(ReadRawUpdate(client, firstIncremental));
    assert(firstIncremental != firstFull);

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
