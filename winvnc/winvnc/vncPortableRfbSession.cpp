// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"

#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbUpdate.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

RfbSessionStats::RfbSessionStats()
    : messagesProcessed(0),
      framebufferUpdatesSent(0),
      setPixelFormatMessages(0),
      setEncodingsMessages(0),
      keyEvents(0),
      pointerEvents(0),
      clientCutTextMessages(0)
{
}

bool RfbServerSession::RunHandshake(TcpSocket& socket, const ServerConfig& config) const
{
    std::string error;
    if (!config.Validate(&error)) {
        return false;
    }

    const std::string version = ProtocolVersion38();
    if (!socket.WriteAll(version.data(), version.size())) {
        return false;
    }

    char clientVersion[sz_rfbProtocolVersionMsg] = {};
    if (!socket.ReadExact(clientVersion, sizeof(clientVersion))) {
        return false;
    }
    if (!IsProtocolVersionMessage(std::string(clientVersion, sizeof(clientVersion)))) {
        return false;
    }

    const std::vector<CARD8> security = NoAuthSecurityTypes();
    if (!socket.WriteAll(security.data(), security.size())) {
        return false;
    }

    CARD8 selectedSecurity = 0;
    if (!socket.ReadExact(&selectedSecurity, sizeof(selectedSecurity)) || selectedSecurity != rfbNoAuth) {
        return false;
    }

    const CARD32 authOk = AuthOkValue();
    if (!socket.WriteAll(&authOk, sizeof(authOk))) {
        return false;
    }

    rfbClientInitMsg clientInit;
    if (!socket.ReadExact(&clientInit, sz_rfbClientInitMsg)) {
        return false;
    }

    const std::vector<CARD8> init = ServerInitBytes(config.Width(), config.Height(), config.PixelFormat(), config.DesktopName());
    return socket.WriteAll(init.data(), init.size());
}

bool RfbServerSession::ServeFramebufferUpdateRequest(TcpSocket& socket, const Framebuffer& framebuffer) const
{
    rfbFramebufferUpdateRequestMsg wire;
    if (!socket.ReadExact(&wire, sz_rfbFramebufferUpdateRequestMsg)) {
        return false;
    }

    FramebufferUpdateRequest request;
    if (!DecodeFramebufferUpdateRequest(wire, request)) {
        return false;
    }

    const std::vector<CARD8> update = RawFramebufferUpdateBytes(framebuffer, request);
    return socket.WriteAll(update.data(), update.size());
}

bool RfbServerSession::ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental) const
{
    updateSent = false;
    CARD8 type = 0;
    if (!socket.ReadExact(&type, sizeof(type))) {
        return false;
    }

    if (stats) {
        stats->messagesProcessed += 1;
    }

    switch (type) {
    case rfbFramebufferUpdateRequest: {
        rfbFramebufferUpdateRequestMsg wire;
        wire.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbFramebufferUpdateRequestMsg - 1)) {
            return false;
        }
        FramebufferUpdateRequest request;
        if (!DecodeFramebufferUpdateRequest(wire, request)) {
            return false;
        }
        const std::vector<CARD8> update = (request.incremental && !forceRawIncremental) ?
            EmptyFramebufferUpdateBytes() : RawFramebufferUpdateBytes(framebuffer, request);
        updateSent = socket.WriteAll(update.data(), update.size());
        if (updateSent && stats) {
            stats->framebufferUpdatesSent += 1;
        }
        return updateSent;
    }
    case rfbSetPixelFormat: {
        rfbSetPixelFormatMsg wire;
        wire.type = type;
        const bool ok = socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbSetPixelFormatMsg - 1);
        if (ok && state) {
            rfbPixelFormat format;
            if (DecodeSetPixelFormat(wire, format)) {
                state->SetPixelFormat(format);
            }
        }
        if (ok && stats) {
            stats->setPixelFormatMessages += 1;
        }
        return ok;
    }
    case rfbSetEncodings: {
        rfbSetEncodingsMsg wire;
        wire.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbSetEncodingsMsg - 1)) {
            return false;
        }
        unsigned int count = 0;
        if (!DecodeSetEncodingsHeader(wire, count)) {
            return false;
        }
        std::vector<CARD8> payload(count * sizeof(CARD32));
        const bool ok = payload.empty() || socket.ReadExact(payload.data(), payload.size());
        if (ok && state) {
            state->SetEncodings(DecodeSetEncodingsPayload(payload));
        }
        if (ok && stats) {
            stats->setEncodingsMessages += 1;
        }
        return ok;
    }
    case rfbKeyEvent: {
        rfbKeyEventMsg wire;
        wire.type = type;
        const bool ok = socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbKeyEventMsg - 1);
        if (ok) {
            KeyEvent event;
            if (!DecodeKeyEvent(wire, event)) {
                return false;
            }
            if (state) {
                state->RecordKeyEvent(event);
            }
            if (inputSink) {
                std::string inputError;
                if (!inputSink->InjectKey(event, &inputError)) {
                    return false;
                }
            }
        }
        if (ok && stats) {
            stats->keyEvents += 1;
        }
        return ok;
    }
    case rfbPointerEvent: {
        rfbPointerEventMsg wire;
        wire.type = type;
        const bool ok = socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbPointerEventMsg - 1);
        if (ok) {
            PointerEvent event;
            if (!DecodePointerEvent(wire, event)) {
                return false;
            }
            if (state) {
                state->RecordPointerEvent(event);
            }
            if (inputSink) {
                std::string inputError;
                if (!inputSink->InjectPointer(event, &inputError)) {
                    return false;
                }
            }
        }
        if (ok && stats) {
            stats->pointerEvents += 1;
        }
        return ok;
    }
    case rfbClientCutText: {
        rfbClientCutTextMsg wire;
        wire.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbClientCutTextMsg - 1)) {
            return false;
        }
        const CARD32 length = Swap32IfLE(wire.length);
        std::vector<CARD8> payload(length);
        const bool ok = payload.empty() || socket.ReadExact(payload.data(), payload.size());
        if (ok && state) {
            state->RecordClientCutText(length);
        }
        if (ok && stats) {
            stats->clientCutTextMessages += 1;
        }
        return ok;
    }
    default:
        return false;
    }
}

bool RfbServerSession::ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental) const
{
    for (unsigned int i = 0; i < maxMessages; ++i) {
        bool updateSent = false;
        if (!ServeNextClientMessage(socket, framebuffer, updateSent, stats, state, inputSink, forceRawIncremental)) {
            return false;
        }
        if (updateSent) {
            return true;
        }
    }
    return false;
}

bool RfbServerSession::ServeFramebufferUpdates(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental) const
{
    if (updateCount == 0) {
        return true;
    }

    unsigned int sent = 0;
    for (unsigned int i = 0; i < maxMessages && sent < updateCount; ++i) {
        bool updateSent = false;
        if (!ServeNextClientMessage(socket, framebuffer, updateSent, stats, state, inputSink, forceRawIncremental)) {
            return false;
        }
        if (updateSent) {
            sent += 1;
        }
    }
    return sent == updateCount;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
