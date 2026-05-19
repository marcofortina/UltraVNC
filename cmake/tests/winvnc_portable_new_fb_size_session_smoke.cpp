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

class ResizingSource : public DesktopSource {
public:
    explicit ResizingSource(const rfbPixelFormat& format)
        : format_(format), snapshots_(0)
    {
    }

    rfb::Rect Size() const override { return rfb::Rect(0, 0, snapshots_ == 0 ? 4 : 6, snapshots_ == 0 ? 4 : 5); }
    rfbPixelFormat Format() const override { return format_; }

    bool Snapshot(Framebuffer& destination, rfb::Region2D& changed) override
    {
        const unsigned int width = snapshots_ == 0 ? 4 : 6;
        const unsigned int height = snapshots_ == 0 ? 4 : 5;
        snapshots_ += 1;
        if (!destination.Reset(width, height, format_)) {
            return false;
        }
        destination.Fill(0x55);
        changed.reset(destination.Bounds());
        return true;
    }

private:
    rfbPixelFormat format_;
    mutable unsigned int snapshots_;
};

bool ClientHandshake(TcpSocket& client)
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

bool ReadNewFramebufferSize(TcpSocket& client)
{
    rfbFramebufferUpdateMsg update;
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) {
        return false;
    }
    rfbFramebufferUpdateRectHeader rect;
    if (!client.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
        return false;
    }
    return Swap32IfLE(rect.encoding) == rfbEncodingNewFBSize &&
           Swap16IfLE(rect.r.w) == 6 &&
           Swap16IfLE(rect.r.h) == 5;
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(4, 4);

    ResizingSource source(config.PixelFormat());
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
    assert(ClientHandshake(client));

    const std::vector<CARD8> encodings = EncodeSetEncodings(std::vector<CARD32>{rfbEncodingNewFBSize, rfbEncodingRaw});
    assert(client.WriteAll(encodings.data(), encodings.size()));

    FramebufferUpdateRequest request{true, 0, 0, 4, 4};
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadNewFramebufferSize(client));

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
