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
#include <thread>

using namespace uvnc::winvnc::portable;

class RecordingClipboardSink : public RfbClipboardSink {
public:
    bool SetText(const std::string& text, std::string *) override
    {
        value = text;
        return true;
    }

    std::string value;
};

int main()
{
    auto pair = TcpSocket::CreateConnectedPair();
    assert(pair.first.Valid());
    assert(pair.second.Valid());

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbClientState state;
    RecordingClipboardSink clipboard;
    bool updateSent = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(pair.first, framebuffer, updateSent, nullptr, &state, nullptr, false, &clipboard);
    });

    const std::vector<CARD8> cut = EncodeClientCutText("linux clipboard");
    assert(pair.second.WriteAll(cut.data(), cut.size()));

    worker.join();
    assert(serverOk);
    assert(!updateSent);
    assert(state.ClientCutTextMessages() == 1);
    assert(state.LastClientCutText() == "linux clipboard");
    assert(clipboard.value == "linux clipboard");
    return 0;
}
