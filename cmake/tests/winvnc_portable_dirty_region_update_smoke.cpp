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
#include <vector>

using namespace uvnc::winvnc::portable;

namespace {

class DirtyRectSource : public DesktopSource {
public:
    explicit DirtyRectSource(const rfbPixelFormat& format)
        : format_(format), fill_(0x40)
    {
    }

    rfb::Rect Size() const override { return rfb::Rect(0, 0, 8, 8); }
    rfbPixelFormat Format() const override { return format_; }

    bool Snapshot(Framebuffer& destination, rfb::Region2D& changed) override
    {
        if (!destination.Reset(8, 8, format_)) {
            return false;
        }
        destination.Fill(fill_++);
        changed.reset(rfb::Rect(2, 1, 5, 4));
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
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != AuthOkValue()) return false;
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    std::vector<CARD8> name(Swap32IfLE(serverInit.nameLength));
    return name.empty() || client.ReadExact(name.data(), name.size());
}

bool ReadDirtyUpdate(TcpSocket& client)
{
    rfbFramebufferUpdateMsg update;
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) {
        return false;
    }
    rfbFramebufferUpdateRectHeader rect;
    if (!client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
        return false;
    }
    if (Swap16IfLE(rect.r.x) != 2 || Swap16IfLE(rect.r.y) != 1 ||
        Swap16IfLE(rect.r.w) != 3 || Swap16IfLE(rect.r.h) != 3 ||
        Swap32IfLE(rect.encoding) != rfbEncodingRaw) {
        return false;
    }
    std::vector<CARD8> pixels(3 * 3 * 4);
    return client.ReadExact(pixels.data(), pixels.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(8, 8);

    DirtyRectSource source(config.PixelFormat());
    Framebuffer initial;
    rfb::Region2D changed;
    assert(source.Snapshot(initial, changed));

    MemoryServer server;
    assert(server.StartWithFramebuffer(config, initial));

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdatesFromSource(source, 1);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(ClientHandshake(client, config));

    FramebufferUpdateRequest incremental{true, 0, 0, 8, 8};
    const rfbFramebufferUpdateRequestMsg request = EncodeFramebufferUpdateRequest(incremental);
    assert(client.WriteAll(&request, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadDirtyUpdate(client));

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
