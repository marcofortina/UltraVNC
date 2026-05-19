// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"

#include "vncPortableRfb.h"
#include "vncPortableCursor.h"
#include "vncPortableFileTransfer.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbUpdate.h"

#include <cstring>
#include <random>
#include <string>
#include <vector>

extern "C" {
#include "d3des.h"
}

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

void EncryptVncAuthChallenge(std::vector<CARD8>& challenge, const std::string& password)
{
    unsigned char key[8] = {};
    for (std::size_t i = 0; i < sizeof(key) && i < password.size(); ++i) {
        key[i] = static_cast<unsigned char>(password[i]);
    }
    deskey(key, EN0);
    for (std::size_t i = 0; i + 8 <= challenge.size(); i += 8) {
        des(challenge.data() + i, challenge.data() + i);
    }
}


bool MaybeSendClipboardSource(TcpSocket& socket, RfbClientState *state, RfbClipboardSource *clipboardSource)
{
    if (!state || !clipboardSource) {
        return true;
    }
    std::string text;
    std::string error;
    if (!clipboardSource->GetText(text, &error)) {
        return false;
    }
    if (text.empty() || text == state->LastServerCutText()) {
        return true;
    }
    const std::vector<CARD8> bytes = EncodeServerCutText(text);
    if (!socket.WriteAll(bytes.data(), bytes.size())) {
        return false;
    }
    state->RecordServerCutTextSent(text);
    return true;
}

std::vector<CARD8> GenerateVncAuthChallenge()
{
    std::vector<CARD8> challenge(16);
    std::random_device random;
    for (std::size_t i = 0; i < challenge.size(); ++i) {
        challenge[i] = static_cast<CARD8>(random());
    }
    return challenge;
}

bool RunVncPasswordAuthentication(TcpSocket& socket, const std::string& password)
{
    std::vector<CARD8> challenge = GenerateVncAuthChallenge();
    if (!socket.WriteAll(challenge.data(), challenge.size())) {
        return false;
    }

    std::vector<CARD8> response(challenge.size());
    if (!socket.ReadExact(response.data(), response.size())) {
        return false;
    }

    std::vector<CARD8> expected = challenge;
    EncryptVncAuthChallenge(expected, password);
    const CARD32 authResult = std::memcmp(response.data(), expected.data(), expected.size()) == 0 ?
        AuthOkValue() : AuthFailedValue();
    if (!socket.WriteAll(&authResult, sizeof(authResult))) {
        return false;
    }
    return authResult == AuthOkValue();
}

} // namespace

RfbSessionStats::RfbSessionStats()
    : messagesProcessed(0),
      framebufferUpdatesSent(0),
      setPixelFormatMessages(0),
      setEncodingsMessages(0),
      keyEvents(0),
      pointerEvents(0),
      clientCutTextMessages(0),
      pointerPositionUpdatesSent(0),
      fileTransferMessages(0),
      fileTransferBytesDiscarded(0)
{
}

bool RfbServerSession::RunHandshake(TcpSocket& socket, const ServerConfig& config, RfbClientState *state) const
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

    const std::vector<CARD8> security = SecurityTypesForAuthMode(config.AuthMode());
    if (!socket.WriteAll(security.data(), security.size())) {
        return false;
    }

    CARD8 selectedSecurity = 0;
    const CARD8 expectedSecurity = config.AuthMode() == ServerAuthMode::VncPassword ? rfbVncAuth : rfbNoAuth;
    if (!socket.ReadExact(&selectedSecurity, sizeof(selectedSecurity)) || selectedSecurity != expectedSecurity) {
        return false;
    }

    if (config.AuthMode() == ServerAuthMode::VncPassword) {
        if (!RunVncPasswordAuthentication(socket, config.VncPassword())) {
            return false;
        }
    } else {
        const CARD32 authOk = AuthOkValue();
        if (!socket.WriteAll(&authOk, sizeof(authOk))) {
            return false;
        }
    }

    rfbClientInitMsg clientInit;
    if (!socket.ReadExact(&clientInit, sz_rfbClientInitMsg)) {
        return false;
    }
    if (state) {
        state->RecordClientInit((clientInit.flags & clientInitShared) != 0);
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

bool RfbServerSession::ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    updateSent = false;
    if (!MaybeSendClipboardSource(socket, state, clipboardSource)) {
        return false;
    }
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
            EmptyFramebufferUpdateBytes() :
            (state ? EncodedFramebufferUpdateBytes(framebuffer, request, state->PixelFormat(), state->Encodings()) :
                     RawFramebufferUpdateBytes(framebuffer, request));
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
            if (state->SupportsCursorShapeUpdates() && !SendCursorShape(socket, *state)) {
                return false;
            }
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
            if (state && state->SupportsPointerPositionUpdates()) {
                const std::vector<CARD8> pointerUpdate = PointerPositionUpdateBytes(event.x, event.y);
                if (!socket.WriteAll(pointerUpdate.data(), pointerUpdate.size())) {
                    return false;
                }
                updateSent = true;
                if (stats) {
                    stats->pointerPositionUpdatesSent += 1;
                    stats->framebufferUpdatesSent += 1;
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
            const std::string text(reinterpret_cast<const char *>(payload.data()), payload.size());
            state->RecordClientCutText(text);
            if (clipboardSink) {
                std::string clipboardError;
                if (!clipboardSink->SetText(text, &clipboardError)) {
                    return false;
                }
            }
        }
        if (ok && stats) {
            stats->clientCutTextMessages += 1;
        }
        return ok;
    }
    case rfbFileTransfer: {
        rfbFileTransferMsg wire;
        wire.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbFileTransferMsg - 1)) {
            return false;
        }
        FileTransferMessage message;
        if (!DecodeFileTransferHeader(wire, message)) {
            return false;
        }
        const FileTransferMode mode = state ? state->FileTransferModeValue() : FileTransferMode::Disabled;
        const CARD32 limit = state ? state->FileTransferPayloadLimit() : DefaultFileTransferPayloadLimit();
        const FileTransferDecision decision = EvaluateFileTransferMessage(message, mode, limit);
        if (!decision.readPayload) {
            if (stats) {
                stats->fileTransferMessages += 1;
            }
            return SendFileTransferAbort(socket, decision.abortReason);
        }
        std::vector<CARD8> payload(decision.payloadBytes);
        if (!payload.empty() && !socket.ReadExact(payload.data(), payload.size())) {
            return false;
        }
        if (state && FileTransferMessageMayCarryPath(message.contentType) && !state->FileTransferRoot().empty()) {
            const std::string requestedPath(reinterpret_cast<const char *>(payload.data()), payload.size());
            std::string resolvedPath;
            std::string pathError;
            if (!ResolveFileTransferPath(state->FileTransferRoot(), requestedPath, resolvedPath, &pathError)) {
                if (stats) {
                    stats->fileTransferMessages += 1;
                    stats->fileTransferBytesDiscarded += decision.payloadBytes;
                }
                return SendFileTransferAbort(socket, 0, rfbRErrorCmd);
            }
        }
        if (stats) {
            stats->fileTransferMessages += 1;
            stats->fileTransferBytesDiscarded += decision.payloadBytes;
        }
        return SendFileTransferAbort(socket, decision.abortReason);
    }
    default:
        return false;
    }
}

bool RfbServerSession::ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    for (unsigned int i = 0; i < maxMessages; ++i) {
        bool updateSent = false;
        if (!ServeNextClientMessage(socket, framebuffer, updateSent, stats, state, inputSink, forceRawIncremental, clipboardSink, clipboardSource)) {
            return false;
        }
        if (updateSent) {
            return true;
        }
    }
    return false;
}

bool RfbServerSession::ServeFramebufferUpdates(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    if (updateCount == 0) {
        return true;
    }

    unsigned int sent = 0;
    for (unsigned int i = 0; i < maxMessages && sent < updateCount; ++i) {
        bool updateSent = false;
        if (!ServeNextClientMessage(socket, framebuffer, updateSent, stats, state, inputSink, forceRawIncremental, clipboardSink, clipboardSource)) {
            return false;
        }
        if (updateSent) {
            sent += 1;
        }
    }
    return sent == updateCount;
}

bool RfbServerSession::SendBell(TcpSocket& socket) const
{
    const std::vector<CARD8> bytes = EncodeBell();
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool RfbServerSession::SendServerCutText(TcpSocket& socket, const std::string& text) const
{
    const std::vector<CARD8> bytes = EncodeServerCutText(text);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool RfbServerSession::SendCursorShape(TcpSocket& socket, RfbClientState& state) const
{
    if (!state.SupportsCursorShapeUpdates()) {
        return true;
    }
    const CursorShape cursor = DefaultArrowCursorShape();
    std::vector<CARD8> bytes;
    if (state.SupportsRichCursorUpdates()) {
        bytes = cursor.Valid() ? EncodeRichCursorShapeUpdate(cursor) : EncodeEmptyCursorShapeUpdate(rfbEncodingRichCursor);
    } else if (state.SupportsXCursorUpdates()) {
        bytes = cursor.Valid() ? EncodeXCursorShapeUpdate(cursor) : EncodeEmptyCursorShapeUpdate(rfbEncodingXCursor);
    }
    if (bytes.empty()) {
        return false;
    }
    if (!socket.WriteAll(bytes.data(), bytes.size())) {
        return false;
    }
    state.MarkCursorShapeSent();
    return true;
}

bool RfbServerSession::SendFileTransferAbort(TcpSocket& socket, CARD16 contentParam, CARD32 size) const
{
    const std::vector<CARD8> bytes = EncodeFileTransferAbort(contentParam, size);
    return socket.WriteAll(bytes.data(), bytes.size());
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
