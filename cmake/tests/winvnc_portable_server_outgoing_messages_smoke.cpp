// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"
#include "vncPortableRfbSession.h"
#include "vncPortableTcp.h"

#include <cassert>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string text = "portable clipboard";
    const std::vector<CARD8> cutText = EncodeServerCutText(text);
    assert(cutText.size() == sz_rfbServerCutTextMsg + text.size());
    const auto *header = reinterpret_cast<const rfbServerCutTextMsg *>(cutText.data());
    assert(header->type == rfbServerCutText);
    assert(Swap32IfLE(header->length) == text.size());
    assert(std::string(reinterpret_cast<const char *>(cutText.data() + sz_rfbServerCutTextMsg), text.size()) == text);

    const std::vector<CARD8> bell = EncodeBell();
    assert(bell.size() == sz_rfbBellMsg);
    assert(bell[0] == rfbBell);

    TcpListener listener;
    assert(listener.Listen("127.0.0.1", 0));

    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket accepted;
        serverOk = listener.Accept(accepted) &&
                   RfbServerSession().SendBell(accepted) &&
                   RfbServerSession().SendServerCutText(accepted, text);
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", listener.Port(), client));
    CARD8 bellType = 0;
    assert(client.ReadExact(&bellType, sizeof(bellType)));
    assert(bellType == rfbBell);

    rfbServerCutTextMsg wire;
    assert(client.ReadExact(&wire, sz_rfbServerCutTextMsg));
    assert(wire.type == rfbServerCutText);
    std::string payload(Swap32IfLE(wire.length), '\0');
    assert(client.ReadExact(&payload[0], payload.size()));
    assert(payload == text);

    server.join();
    listener.Close();
    assert(serverOk);
    return 0;
}
