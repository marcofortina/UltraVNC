// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExtendedClipboard.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbSession.h"

#include <cassert>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

namespace {

class RecordingClipboardSink : public RfbClipboardSink {
public:
    bool SetText(const std::string& text, std::string *) override
    {
        value = text;
        return true;
    }
    std::string value;
};

class StaticClipboardSource : public RfbClipboardSource {
public:
    explicit StaticClipboardSource(const std::string& value) : value_(value) {}
    bool GetText(std::string& text, std::string *) const override
    {
        text = value_;
        return true;
    }
private:
    std::string value_;
};

bool ReadExtendedServerCutText(TcpSocket& socket, ExtendedClipboardPayload& payload)
{
    rfbServerCutTextMsg header = {};
    if (!socket.ReadExact(&header, sz_rfbServerCutTextMsg) || header.type != rfbServerCutText) {
        return false;
    }
    if (!IsExtendedClipboardWireLength(header.length)) {
        return false;
    }
    std::vector<CARD8> bytes(ExtendedClipboardPayloadLength(header.length));
    if (!bytes.empty() && !socket.ReadExact(bytes.data(), bytes.size())) {
        return false;
    }
    return DecodeExtendedClipboardPayload(bytes, payload);
}

} // namespace

int main()
{
    int fds[2] = {-1, -1};
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TcpSocket serverSocket(fds[0]);
    TcpSocket clientSocket(fds[1]);

    RfbServerSession session;
    Framebuffer framebuffer(8, 8, ServerConfig::DefaultPixelFormat());
    RfbClientState state;
    RfbSessionStats stats;
    bool updateSent = false;

    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state);
    });

    const std::vector<CARD8> setEncodings = EncodeSetEncodings(std::vector<CARD32>(1, rfbEncodingExtendedClipboard));
    assert(clientSocket.WriteAll(setEncodings.data(), setEncodings.size()));
    ExtendedClipboardPayload serverPayload;
    assert(ReadExtendedServerCutText(clientSocket, serverPayload));
    assert((serverPayload.flags & clipCaps) != 0);
    assert((serverPayload.flags & clipText) != 0);
    worker.join();
    assert(serverOk);
    assert(state.SupportsExtendedClipboard());
    assert(state.ExtendedClipboardCapsSent());
    assert(stats.extendedClipboardCapsSent == 1);

    RecordingClipboardSink sink;
    serverOk = false;
    std::thread provideWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr, false, &sink);
    });
    const std::vector<CARD8> provide = EncodeExtendedClientCutText(EncodeExtendedClipboardProvideText("client extended text"));
    assert(clientSocket.WriteAll(provide.data(), provide.size()));
    provideWorker.join();
    assert(serverOk);
    assert(sink.value == "client extended text");
    assert(state.LastClientCutText() == "client extended text");

    StaticClipboardSource source("server extended text");
    serverOk = false;
    std::thread requestWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr, false, nullptr, &source);
    });
    const std::vector<CARD8> combinedCaps = EncodeExtendedClientCutText(EncodeExtendedClipboardCaps(clipCaps | clipRequest | clipProvide | clipText, 4096));
    serverOk = false;
    std::thread capsUpdateWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, &stats, &state, nullptr, false, nullptr, &source);
    });
    assert(clientSocket.WriteAll(combinedCaps.data(), combinedCaps.size()));
    capsUpdateWorker.join();
    assert(serverOk);
    assert(state.ExtendedClipboardRemoteTextLimit() == 4096);

    const std::vector<CARD8> request = EncodeExtendedClientCutText(EncodeExtendedClipboardRequest());
    assert(clientSocket.WriteAll(request.data(), request.size()));
    assert(ReadExtendedServerCutText(clientSocket, serverPayload));
    assert((serverPayload.flags & clipNotify) != 0);
    assert(ReadExtendedServerCutText(clientSocket, serverPayload));
    assert((serverPayload.flags & clipProvide) != 0);
    assert(serverPayload.textPresent);
    assert(serverPayload.text == "server extended text");
    requestWorker.join();
    assert(serverOk);
    assert(stats.extendedClipboardMessages >= 2);
    assert(stats.extendedClipboardProvidesSent >= 1);
    return 0;
}
