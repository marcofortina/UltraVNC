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

class FailingCursorSource : public RfbCursorSource {
public:
    bool GetCursorShape(CursorShape& shape, std::string *error) const override
    {
        shape = CursorShape();
        if (error) *error = "synthetic cursor failure";
        return false;
    }
};

bool RunHandshake(TcpSocket& client)
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
    rfbClientInitMsg init = {};
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit = {};
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    std::string name(Swap32IfLE(serverInit.nameLength), '\0');
    return name.empty() || client.ReadExact(&name[0], name.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetPort(0);
    config.SetSize(8, 8);

    MemoryServer server;
    assert(server.Start(config));
    FailingCursorSource cursor;
    bool accepted = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.TryServeOneUpdates(1, nullptr, 0, accepted, nullptr, nullptr, nullptr, &cursor);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));
    assert(RunHandshake(client));
    const std::vector<CARD8> encodings = EncodeSetEncodings(std::vector<CARD32>{rfbEncodingRichCursor, rfbEncodingRaw});
    assert(client.WriteAll(encodings.data(), encodings.size()));

    rfbFramebufferUpdateMsg cursorUpdate = {};
    assert(client.ReadExact(&cursorUpdate, sz_rfbFramebufferUpdateMsg));
    assert(cursorUpdate.type == rfbFramebufferUpdate);
    assert(Swap16IfLE(cursorUpdate.nRects) == 1);
    rfbFramebufferUpdateRectHeader cursorHeader = {};
    assert(client.ReadExact(&cursorHeader, sz_rfbFramebufferUpdateRectHeader));
    assert(Swap32IfLE(cursorHeader.encoding) == rfbEncodingRichCursor);
    assert(Swap16IfLE(cursorHeader.r.w) == 0);
    assert(Swap16IfLE(cursorHeader.r.h) == 0);

    FramebufferUpdateRequest request = {};
    request.width = 8;
    request.height = 8;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));

    rfbFramebufferUpdateMsg raw = {};
    assert(client.ReadExact(&raw, sz_rfbFramebufferUpdateMsg));
    assert(Swap16IfLE(raw.nRects) == 1);

    worker.join();
    server.Stop();
    assert(accepted);
    assert(serverOk);
    return 0;
}
