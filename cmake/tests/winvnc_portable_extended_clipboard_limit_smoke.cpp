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
class LargeClipboardSource : public RfbClipboardSource {
public:
    bool GetText(std::string& text, std::string *) const override
    {
        text = "too-large-for-auto-provide";
        return true;
    }
};

bool ReadExtendedServerCutText(TcpSocket& socket, ExtendedClipboardPayload& payload)
{
    rfbServerCutTextMsg header = {};
    if (!socket.ReadExact(&header, sz_rfbServerCutTextMsg) || header.type != rfbServerCutText) return false;
    if (!IsExtendedClipboardWireLength(header.length)) return false;
    std::vector<CARD8> bytes(ExtendedClipboardPayloadLength(header.length));
    if (!bytes.empty() && !socket.ReadExact(bytes.data(), bytes.size())) return false;
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
    bool updateSent = false;

    bool serverOk = false;
    std::thread capsWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, nullptr, &state);
    });
    std::vector<CARD32> encodings(1, rfbEncodingExtendedClipboard);
    const std::vector<CARD8> setEncodings = EncodeSetEncodings(encodings);
    assert(clientSocket.WriteAll(setEncodings.data(), setEncodings.size()));
    ExtendedClipboardPayload payload;
    assert(ReadExtendedServerCutText(clientSocket, payload));
    capsWorker.join();
    assert(serverOk);

    serverOk = false;
    std::thread remoteCapsWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, nullptr, &state);
    });
    const std::vector<CARD8> remoteCaps = EncodeExtendedClientCutText(EncodeExtendedClipboardCaps(clipCaps | clipRequest | clipProvide | clipText, 4));
    assert(clientSocket.WriteAll(remoteCaps.data(), remoteCaps.size()));
    remoteCapsWorker.join();
    assert(serverOk);
    assert(state.ExtendedClipboardRemoteTextLimit() == 4);

    LargeClipboardSource source;
    serverOk = false;
    std::thread updateWorker([&]() {
        serverOk = session.ServeNextClientMessage(serverSocket, framebuffer, updateSent, nullptr, &state, nullptr, false, nullptr, &source);
    });
    const rfbFramebufferUpdateRequestMsg update = EncodeFramebufferUpdateRequest({false, 0, 0, 8, 8});
    assert(clientSocket.WriteAll(&update, sz_rfbFramebufferUpdateRequestMsg));
    assert(ReadExtendedServerCutText(clientSocket, payload));
    assert((payload.flags & clipNotify) != 0);
    rfbFramebufferUpdateMsg framebufferUpdate = {};
    assert(clientSocket.ReadExact(&framebufferUpdate, sz_rfbFramebufferUpdateMsg));
    updateWorker.join();
    assert(serverOk);
    return 0;
}
