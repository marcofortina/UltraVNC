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

class CyclingCursorSource : public RfbCursorSource {
public:
    bool GetCursorShape(CursorShape& shape, std::string *) const override
    {
        shape = DefaultArrowCursorShape();
        if (calls_++ > 0) {
            const std::size_t offset = (static_cast<std::size_t>(1) * shape.width + 1) * 4;
            shape.bgra[offset + 0] = 64;
            shape.bgra[offset + 1] = 128;
            shape.bgra[offset + 2] = 255;
            shape.bgra[offset + 3] = 255;
        }
        return true;
    }
private:
    mutable unsigned int calls_ = 0;
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

bool ReadRichCursor(TcpSocket& client)
{
    rfbFramebufferUpdateMsg update = {};
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || update.type != rfbFramebufferUpdate || Swap16IfLE(update.nRects) != 1) return false;
    rfbFramebufferUpdateRectHeader header = {};
    if (!client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader) || Swap32IfLE(header.encoding) != rfbEncodingRichCursor) return false;
    const unsigned int width = Swap16IfLE(header.r.w);
    const unsigned int height = Swap16IfLE(header.r.h);
    if (width == 0 || height == 0) return false;
    std::vector<CARD8> payload(static_cast<std::size_t>(width) * height * 4 + ((width + 7) / 8) * height);
    return client.ReadExact(payload.data(), payload.size());
}

bool ReadRawUpdate(TcpSocket& client, unsigned int width, unsigned int height)
{
    rfbFramebufferUpdateMsg update = {};
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) return false;
    rfbFramebufferUpdateRectHeader header = {};
    if (!client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader) || Swap32IfLE(header.encoding) != rfbEncodingRaw) return false;
    std::string pixels(width * height * 4, '\0');
    return client.ReadExact(&pixels[0], pixels.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetPort(0);
    config.SetSize(16, 16);

    MemoryServer server;
    assert(server.Start(config));
    CyclingCursorSource cursor;
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
    assert(ReadRichCursor(client));

    FramebufferUpdateRequest request = {};
    request.incremental = false;
    request.width = 16;
    request.height = 16;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    assert(client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadRichCursor(client));
    assert(ReadRawUpdate(client, 16, 16));

    worker.join();
    server.Stop();
    assert(accepted);
    assert(serverOk);
    return 0;
}
