// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"

#include <cassert>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

bool RunClientHandshake(TcpSocket& client, const ServerConfig& config)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version)) || !client.WriteAll(version, sizeof(version))) return false;
    CARD8 count = 0;
    if (!client.ReadExact(&count, sizeof(count)) || count != 1) return false;
    CARD8 security = 0;
    if (!client.ReadExact(&security, sizeof(security)) || security != rfbNoAuth) return false;
    if (!client.WriteAll(&security, sizeof(security))) return false;
    CARD32 auth = 0;
    if (!client.ReadExact(&auth, sizeof(auth)) || auth != AuthOkValue()) return false;
    rfbClientInitMsg init;
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit;
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::vector<CARD8> name(nameLength);
    return name.empty() || client.ReadExact(name.data(), name.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetSize(64, 32);

    MemoryServer server;
    assert(server.Start(config));

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.ServeOneUpdates(1);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(RunClientHandshake(client, config));

    const std::vector<CARD8> encodings = EncodeSetEncodings(std::vector<CARD32>{rfbEncodingRichCursor, rfbEncodingRaw});
    assert(client.WriteAll(encodings.data(), encodings.size()));

    rfbFramebufferUpdateMsg cursorUpdate;
    assert(client.ReadExact(&cursorUpdate, sz_rfbFramebufferUpdateMsg));
    assert(cursorUpdate.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(cursorUpdate.nRects) == 1);
    rfbFramebufferUpdateRectHeader cursorHeader;
    assert(client.ReadExact(&cursorHeader, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap32IfLE(cursorHeader.encoding) == rfbEncodingRichCursor);
    const unsigned int width = Swap16IfLE(cursorHeader.r.w);
    const unsigned int height = Swap16IfLE(cursorHeader.r.h);
    assert(width > 0);
    assert(height > 0);
    std::vector<CARD8> cursorPayload(static_cast<std::size_t>(width) * height * 4 + ((width + 7) / 8) * height);
    assert(client.ReadExact(cursorPayload.data(), cursorPayload.size()));

    FramebufferUpdateRequest request;
    request.incremental = false;
    request.x = 0;
    request.y = 0;
    request.width = config.Width();
    request.height = config.Height();
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg update;
    assert(client.ReadExact(&update, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(update.nRects) == 1);

    worker.join();
    server.Stop();
    assert(serverOk);
    return 0;
}
