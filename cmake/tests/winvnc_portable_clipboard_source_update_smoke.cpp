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
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

class AdvancingClipboardSource : public RfbClipboardSource {
public:
    bool GetText(std::string& text, std::string *) const override
    {
        text = calls_++ == 0 ? "first" : "second";
        return true;
    }
private:
    mutable unsigned int calls_ = 0;
};

bool RunHandshake(TcpSocket& client)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) return false;
    const std::string clientVersion = ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) return false;
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) return false;
    const CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) return false;
    CARD32 securityResult = 1;
    if (!client.ReadExact(&securityResult, sizeof(securityResult)) || securityResult != AuthOkValue()) return false;
    rfbClientInitMsg init = {};
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit = {};
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    std::string name(Swap32IfLE(serverInit.nameLength), '\0');
    return name.empty() || client.ReadExact(&name[0], name.size());
}


bool ReadAndDiscardRawUpdate(TcpSocket& client, unsigned int width, unsigned int height)
{
    rfbFramebufferUpdateMsg update = {};
    if (!client.ReadExact(&update, sz_rfbFramebufferUpdateMsg) || Swap16IfLE(update.nRects) != 1) return false;
    rfbFramebufferUpdateRectHeader header = {};
    if (!client.ReadExact(&header, sz_rfbFramebufferUpdateRectHeader) || Swap32IfLE(header.encoding) != rfbEncodingRaw) return false;
    std::string pixels(width * height * 4, '\0');
    return client.ReadExact(&pixels[0], pixels.size());
}

bool ReadServerCutText(TcpSocket& client, std::string& text)
{
    rfbServerCutTextMsg cut = {};
    if (!client.ReadExact(&cut, sz_rfbServerCutTextMsg) || cut.type != rfbServerCutText) return false;
    text.assign(Swap32IfLE(cut.length), '\0');
    return text.empty() || client.ReadExact(&text[0], text.size());
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

    AdvancingClipboardSource source;
    bool accepted = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.TryServeOneUpdates(2, nullptr, 0, accepted, nullptr, &source);
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) && RunHandshake(client);
    std::string text;
    clientOk = clientOk && ReadServerCutText(client, text) && text == "first";

    FramebufferUpdateRequest request = {};
    request.incremental = false;
    request.width = 16;
    request.height = 16;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    clientOk = clientOk && client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);
    clientOk = clientOk && ReadServerCutText(client, text) && text == "second";
    clientOk = clientOk && ReadAndDiscardRawUpdate(client, 16, 16);

    clientOk = clientOk && client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);
    clientOk = clientOk && ReadAndDiscardRawUpdate(client, 16, 16);

    worker.join();
    server.Stop();
    assert(accepted);
    assert(serverOk);
    assert(clientOk);
    return 0;
}
