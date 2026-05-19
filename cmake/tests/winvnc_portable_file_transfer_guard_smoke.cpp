// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"
#include "vncPortableRfbMessages.h"

#include <cassert>
#include <cstring>
#include <thread>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string payload = "ignored-file-transfer-payload";
    auto pair = TcpSocket::CreateConnectedPair();
    assert(pair.first.Valid());
    assert(pair.second.Valid());

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbSessionStats stats;
    bool updateSent = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(pair.first, framebuffer, updateSent, &stats, nullptr, nullptr);
    });

    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbFileTransferOffer;
    message.contentParam = Swap16IfLE(rfbFileTransferVersion);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));
    assert(pair.second.WriteAll(&message, sz_rfbFileTransferMsg));
    assert(pair.second.WriteAll(payload.data(), payload.size()));

    rfbFileTransferMsg abort;
    assert(pair.second.ReadExact(&abort, sz_rfbFileTransferMsg));
    assert(abort.type == rfbFileTransfer);
    assert(abort.contentType == rfbAbortFileTransfer);

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(stats.fileTransferMessages == 1);
    assert(stats.fileTransferBytesDiscarded == payload.size());
    return 0;
}
