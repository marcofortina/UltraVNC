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
#include "vncPortableExtendedClipboard.h"
#include "vncPortableFileTransfer.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableRfbUpdate.h"
#include "vncPortableRfbTransport.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
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


bool MaybeSendClipboardSource(RfbTransport& socket, RfbClientState *state, RfbClipboardSource *clipboardSource)
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
    std::vector<CARD8> bytes;
    if (state->SupportsExtendedClipboard()) {
        bytes = EncodeExtendedServerCutText(EncodeExtendedClipboardNotify(true));
        if (!socket.WriteAll(bytes.data(), bytes.size())) {
            return false;
        }
        const unsigned int remoteLimit = state->ExtendedClipboardRemoteTextLimit();
        if (remoteLimit != 0 && text.size() > remoteLimit) {
            state->RecordServerCutTextSent(text);
            return true;
        }
        bytes = EncodeExtendedServerCutText(EncodeExtendedClipboardProvideText(text));
    } else {
        bytes = EncodeServerCutText(text);
    }
    if (!socket.WriteAll(bytes.data(), bytes.size())) {
        return false;
    }
    state->RecordServerCutTextSent(text);
    return true;
}


bool SendExtendedClipboardCaps(RfbTransport& socket, RfbClientState& state, RfbSessionStats *stats)
{
    if (!state.SupportsExtendedClipboard() || state.ExtendedClipboardCapsSent()) {
        return true;
    }
    const std::vector<CARD8> payload = EncodeExtendedClipboardCaps(kExtendedClipboardServerCaps, state.ExtendedClipboardTextLimit());
    const std::vector<CARD8> message = EncodeExtendedServerCutText(payload);
    if (!socket.WriteAll(message.data(), message.size())) {
        return false;
    }
    state.MarkExtendedClipboardCapsSent();
    if (stats) {
        stats->extendedClipboardCapsSent += 1;
    }
    return true;
}

bool SendExtendedClipboardNotify(RfbTransport& socket, bool textAvailable, RfbSessionStats *stats)
{
    const std::vector<CARD8> payload = EncodeExtendedClipboardNotify(textAvailable);
    const std::vector<CARD8> message = EncodeExtendedServerCutText(payload);
    if (!socket.WriteAll(message.data(), message.size())) {
        return false;
    }
    if (stats) {
        stats->extendedClipboardNotifiesSent += 1;
    }
    return true;
}

bool SendExtendedClipboardProvide(RfbTransport& socket, const std::string& text, RfbSessionStats *stats)
{
    const std::vector<CARD8> payload = EncodeExtendedClipboardProvideText(text);
    const std::vector<CARD8> message = EncodeExtendedServerCutText(payload);
    if (!socket.WriteAll(message.data(), message.size())) {
        return false;
    }
    if (stats) {
        stats->extendedClipboardProvidesSent += 1;
    }
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

bool IsReadFileTransferMode(FileTransferMode mode)
{
    return mode == FileTransferMode::ReadOnly || mode == FileTransferMode::ReadWrite;
}

bool IsWriteFileTransferMode(FileTransferMode mode)
{
    return mode == FileTransferMode::ReadWrite;
}

bool SendFileTransferAccess(RfbTransport& socket, bool allowed)
{
    const std::vector<CARD8> bytes = EncodeFileTransferAccess(allowed);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool SendFileTransferPacketMessage(RfbTransport& socket, CARD8 contentType, CARD16 contentParam, CARD32 size, const std::vector<CARD8>& payload)
{
    const std::vector<CARD8> bytes = EncodeFileTransferPacket(contentType, contentParam, size, payload);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool SendFileTransferPacketMessage(RfbTransport& socket, CARD8 contentType, CARD16 contentParam, CARD32 size, const std::string& payload)
{
    const std::vector<CARD8> bytes = EncodeFileTransferPacket(contentType, contentParam, size, payload);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool SendFileTransferError(RfbTransport& socket)
{
    const std::vector<CARD8> bytes = EncodeFileTransferAbort(0, static_cast<CARD32>(rfbRErrorCmd));
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool SendFileTransferCommandResult(RfbTransport& socket, const FileTransferCommandResult& result)
{
    return SendFileTransferPacketMessage(socket, rfbCommandReturn, result.responseParam, result.status, result.payload);
}

bool SendDirectoryListing(RfbTransport& socket,
                          const std::string& root,
                          const std::string& requestedPath)
{
    std::vector<FileTransferDirectoryEntry> entries;
    std::string reason;
    if (!ListFileTransferDirectory(root, requestedPath, entries, &reason)) {
        return SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADirInaccessible, 0, std::vector<CARD8>());
    }

    if (!SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADirectory, 0, requestedPath)) {
        return false;
    }
    for (std::vector<FileTransferDirectoryEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        const CARD16 param = it->inaccessible ? rfbADirInaccessible : (it->directory ? rfbADirectory : rfbAFile);
        if (!SendFileTransferPacketMessage(socket, rfbDirPacket, param, it->size, it->name)) {
            return false;
        }
    }
    return SendFileTransferPacketMessage(socket, rfbDirPacket, 0, 0, std::vector<CARD8>());
}


bool SendLinuxDrivesList(RfbTransport& socket, const std::string& root)
{
    // UltraVNC's Windows server returns drive roots. On Linux, expose the configured
    // file-transfer root as the only traversable root and include / only as a label.
    if (!SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADrivesList, 0, root.empty() ? std::string("/") : root)) {
        return false;
    }
    return SendFileTransferPacketMessage(socket, rfbDirPacket, 0, 0, std::vector<CARD8>());
}

bool SendRecursiveDirectoryListing(RfbTransport& socket,
                                   const std::string& root,
                                   const std::string& requestedPath,
                                   unsigned int maxDepth,
                                   unsigned int maxEntries)
{
    std::vector<FileTransferRecursiveEntry> entries;
    std::string reason;
    if (!ListFileTransferDirectoryRecursive(root, requestedPath, maxDepth, maxEntries, entries, &reason)) {
        return SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADirInaccessible, 0, std::vector<CARD8>());
    }
    for (std::vector<FileTransferRecursiveEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        const CARD16 param = it->inaccessible ? rfbADirInaccessible : (it->directory ? rfbADirectory : rfbADirRecursiveListItem);
        if (!SendFileTransferPacketMessage(socket, rfbDirPacket, param, it->size, it->relativePath)) {
            return false;
        }
    }
    return SendFileTransferPacketMessage(socket, rfbDirPacket, 0, 0, std::vector<CARD8>());
}

bool SendRecursiveDirectorySize(RfbTransport& socket,
                                const std::string& root,
                                const std::string& requestedPath,
                                unsigned int maxDepth,
                                unsigned int maxEntries)
{
    FileTransferRecursiveSize size;
    std::string reason;
    if (!MeasureFileTransferDirectoryRecursive(root, requestedPath, maxDepth, maxEntries, size, &reason)) {
        return SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADirInaccessible, 0, std::vector<CARD8>());
    }
    std::ostringstream payload;
    payload << "files=" << size.files
            << ";directories=" << size.directories
            << ";bytes=" << size.bytesLow
            << ";truncated=" << (size.truncated ? "true" : "false");
    return SendFileTransferPacketMessage(socket, rfbDirPacket, rfbADirRecursiveSize, size.bytesLow, payload.str());
}


bool SendFileTransferChecksums(RfbTransport& socket,
                               const std::string& root,
                               const std::string& requestedPath)
{
    std::vector<FileTransferChecksumBlock> blocks;
    std::string reason;
    if (!ComputeFileTransferChecksums(root, requestedPath, static_cast<CARD32>(sz_rfbBlockSize), 16384, blocks, &reason)) {
        return SendFileTransferError(socket);
    }
    std::ostringstream payload;
    for (std::vector<FileTransferChecksumBlock>::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
        payload << it->offset << ':' << it->length << ':' << it->crc32 << '\n';
    }
    return SendFileTransferPacketMessage(socket, rfbFileChecksums, 0, static_cast<CARD32>(blocks.size()), payload.str());
}

bool SendFileDownload(RfbTransport& socket, const std::string& path, const std::string& displayPath, CARD32 payloadLimit)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        return SendFileTransferError(socket);
    }
    input.seekg(0, std::ios::end);
    const std::ifstream::pos_type end = input.tellg();
    if (end < 0 || static_cast<unsigned long long>(end) > std::numeric_limits<CARD32>::max()) {
        return SendFileTransferError(socket);
    }
    const CARD32 fileSize = static_cast<CARD32>(end);
    input.seekg(0, std::ios::beg);

    if (!SendFileTransferPacketMessage(socket, rfbFileHeader, rfbAFile, fileSize, displayPath)) {
        return false;
    }

    const CARD32 chunkLimit = payloadLimit == 0 ? static_cast<CARD32>(sz_rfbBlockSize) : payloadLimit;
    const CARD32 chunkSize = std::min<CARD32>(static_cast<CARD32>(sz_rfbBlockSize), chunkLimit);
    std::vector<CARD8> chunk(chunkSize == 0 ? static_cast<CARD32>(sz_rfbBlockSize) : chunkSize);
    CARD32 offset = 0;
    while (input && offset < fileSize) {
        const CARD32 remaining = fileSize - offset;
        const CARD32 toRead = std::min<CARD32>(static_cast<CARD32>(chunk.size()), remaining);
        input.read(reinterpret_cast<char *>(chunk.data()), toRead);
        const std::streamsize got = input.gcount();
        if (got <= 0) {
            return SendFileTransferError(socket);
        }
        std::vector<CARD8> payload(chunk.begin(), chunk.begin() + got);
        if (!SendFileTransferPacketMessage(socket, rfbFilePacket, 0, offset, payload)) {
            return false;
        }
        offset += static_cast<CARD32>(got);
    }

    return SendFileTransferPacketMessage(socket, rfbEndOfFile, 0, fileSize, std::vector<CARD8>());
}

bool TruncateUploadTarget(const std::string& path)
{
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    return static_cast<bool>(output);
}

bool AppendUploadPayload(const std::string& path, const std::vector<CARD8>& payload)
{
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::app);
    if (!output) {
        return false;
    }
    if (!payload.empty()) {
        output.write(reinterpret_cast<const char *>(payload.data()), static_cast<std::streamsize>(payload.size()));
    }
    return static_cast<bool>(output);
}

bool RunVncPasswordAuthentication(RfbTransport& socket, const std::string& password)
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



namespace {

bool RunVncPasswordSecurity(RfbTransport& transport, const ServerConfig& config)
{
    if (config.AuthMode() == ServerAuthMode::VncPassword) {
        return RunVncPasswordAuthentication(transport, config.VncPassword());
    }
    const CARD32 authOk = AuthOkValue();
    return transport.WriteAll(&authOk, sizeof(authOk));
}

bool RunClientInitAndServerInit(RfbTransport& transport, const ServerConfig& config, RfbClientState *state)
{
    rfbClientInitMsg clientInit;
    if (!transport.ReadExact(&clientInit, sz_rfbClientInitMsg)) {
        return false;
    }
    if (state) {
        state->RecordClientInit((clientInit.flags & clientInitShared) != 0);
    }

    const std::vector<CARD8> init = ServerInitBytes(config.Width(), config.Height(), config.PixelFormat(), config.DesktopName());
    return transport.WriteAll(init.data(), init.size());
}

bool RunVeNCryptX509VncNegotiation(RfbTransport& transport)
{
    const CARD16 version = Swap16IfLE(static_cast<CARD16>(kVeNCryptVersion));
    if (!transport.WriteAll(&version, sizeof(version))) {
        return false;
    }

    CARD16 selectedVersion = 0;
    if (!transport.ReadExact(&selectedVersion, sizeof(selectedVersion)) ||
        Swap16IfLE(selectedVersion) != kVeNCryptVersion) {
        const CARD8 failed = 1;
        transport.WriteAll(&failed, sizeof(failed));
        return false;
    }

    const CARD8 ok = 0;
    if (!transport.WriteAll(&ok, sizeof(ok))) {
        return false;
    }

    const CARD8 subtypeCount = 1;
    const CARD32 subtype = Swap32IfLE(kVeNCryptSubTypeX509Vnc);
    if (!transport.WriteAll(&subtypeCount, sizeof(subtypeCount)) ||
        !transport.WriteAll(&subtype, sizeof(subtype))) {
        return false;
    }

    CARD32 selectedSubtype = 0;
    if (!transport.ReadExact(&selectedSubtype, sizeof(selectedSubtype)) ||
        Swap32IfLE(selectedSubtype) != kVeNCryptSubTypeX509Vnc) {
        return false;
    }

    const CARD8 accepted = 1;
    return transport.WriteAll(&accepted, sizeof(accepted));
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
      extendedClipboardMessages(0),
      extendedClipboardCapsSent(0),
      extendedClipboardProvidesSent(0),
      extendedClipboardNotifiesSent(0),
      pointerPositionUpdatesSent(0),
      fileTransferMessages(0),
      fileTransferBytesDiscarded(0)
{
}

bool RfbServerSession::RunHandshake(RfbTransport& socket, const ServerConfig& config, RfbClientState *state) const
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

    if (config.TransportSecurity() != TransportSecurityMode::None) {
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

    return RunVncPasswordSecurity(socket, config) &&
           RunClientInitAndServerInit(socket, config, state);
}

bool RfbServerSession::ServeFramebufferUpdateRequest(RfbTransport& socket, const Framebuffer& framebuffer) const
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

bool RfbServerSession::ServeNextClientMessage(RfbTransport& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
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
            if (!SendExtendedClipboardCaps(socket, *state, stats)) {
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
        if (IsExtendedClipboardWireLength(wire.length)) {
            if (!state || !state->SupportsExtendedClipboard()) {
                return false;
            }
            const unsigned int length = ExtendedClipboardPayloadLength(wire.length);
            if (length == 0 || length > kExtendedClipboardDefaultTextLimit + 4096U) {
                return false;
            }
            std::vector<CARD8> payload(length);
            const bool ok = payload.empty() || socket.ReadExact(payload.data(), payload.size());
            if (!ok) {
                return false;
            }
            ExtendedClipboardPayload extended;
            if (!DecodeExtendedClipboardPayload(payload, extended)) {
                return false;
            }
            const CARD32 action = extended.flags & clipActionMask;
            if (state) {
                if (action == clipCaps || (extended.flags & clipCaps)) {
                    state->RecordExtendedClipboardRemoteCaps(extended.flags, extended.textLimit);
                } else if (action == clipNotify) {
                    state->RecordExtendedClipboardNotify(extended.flags);
                }
            }
            if (action == clipProvide && extended.textPresent) {
                if (state) {
                    state->RecordClientCutText(extended.text);
                }
                if (clipboardSink) {
                    std::string clipboardError;
                    if (!clipboardSink->SetText(extended.text, &clipboardError)) {
                        return false;
                    }
                }
            } else if ((action == clipRequest || action == clipPeek) && clipboardSource) {
                std::string text;
                std::string clipboardError;
                if (!clipboardSource->GetText(text, &clipboardError)) {
                    return false;
                }
                if (action == clipPeek) {
                    if (!SendExtendedClipboardNotify(socket, !text.empty(), stats)) {
                        return false;
                    }
                } else if (!text.empty()) {
                    if (!SendExtendedClipboardProvide(socket, text, stats)) {
                        return false;
                    }
                    if (state) {
                        state->RecordServerCutTextSent(text);
                    }
                }
            }
            if (stats) {
                stats->clientCutTextMessages += 1;
                stats->extendedClipboardMessages += 1;
            }
            return true;
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
        if (stats) {
            stats->fileTransferMessages += 1;
        }

        if (message.contentType == rfbFileTransferAccess || message.contentType == rfbFileTransferProtocolVersion) {
            return SendFileTransferAccess(socket, mode != FileTransferMode::Disabled && mode != FileTransferMode::RejectOnly);
        }
        if (message.contentType == rfbFileTransferSessionStart || message.contentType == rfbFileTransferSessionEnd) {
            if (message.contentType == rfbFileTransferSessionEnd && state) {
                AbortFileTransferUpload(state->FileUploadTemporaryPath());
                state->EndFileUpload();
            }
            return true;
        }
        if (!decision.accepted || !state) {
            if (stats) {
                stats->fileTransferBytesDiscarded += decision.payloadBytes;
            }
            return SendFileTransferAbort(socket, decision.abortReason);
        }

        const std::string requestedPath(reinterpret_cast<const char *>(payload.data()), payload.size());
        std::string resolvedPath;
        std::string pathError;
        if (FileTransferMessageMayCarryPath(message.contentType) &&
            !ResolveFileTransferPath(state->FileTransferRoot(), requestedPath, resolvedPath, &pathError)) {
            if (stats) {
                stats->fileTransferBytesDiscarded += decision.payloadBytes;
            }
            return SendFileTransferAbort(socket, 0, static_cast<CARD32>(rfbRErrorCmd));
        }

        switch (message.contentType) {
        case rfbDirContentRequest:
            if (!IsReadFileTransferMode(mode)) {
                return SendFileTransferAccess(socket, false);
            }
            if (message.contentParam != rfbRDirContent &&
                message.contentParam != rfbRDrivesList &&
                message.contentParam != rfbRDirRecursiveList &&
                message.contentParam != rfbRDirRecursiveSize) {
                return SendFileTransferError(socket);
            }
            if (message.contentParam == rfbRDrivesList) {
                return SendLinuxDrivesList(socket, state->FileTransferRoot());
            }
            if (message.contentParam == rfbRDirRecursiveList) {
                return SendRecursiveDirectoryListing(socket, state->FileTransferRoot(), requestedPath, state->FileTransferRecursiveMaxDepth(), state->FileTransferRecursiveMaxEntries());
            }
            if (message.contentParam == rfbRDirRecursiveSize) {
                return SendRecursiveDirectorySize(socket, state->FileTransferRoot(), requestedPath, state->FileTransferRecursiveMaxDepth(), state->FileTransferRecursiveMaxEntries());
            }
            return SendDirectoryListing(socket, state->FileTransferRoot(), requestedPath);
        case rfbFileTransferRequest:
            if (!IsReadFileTransferMode(mode)) {
                return SendFileTransferAccess(socket, false);
            }
            return SendFileDownload(socket, resolvedPath, requestedPath, limit);
        case rfbFileChecksums:
            if (!IsReadFileTransferMode(mode)) {
                return SendFileTransferAccess(socket, false);
            }
            return SendFileTransferChecksums(socket, state->FileTransferRoot(), requestedPath);
        case rfbFileTransferOffer:
            if (!IsWriteFileTransferMode(mode)) {
                return SendFileTransferAccess(socket, false);
            }
            {
                std::string finalPath;
                std::string temporaryPath;
                std::string uploadError;
                if (!PrepareFileTransferUpload(state->FileTransferRoot(), requestedPath, state->FileTransferAllowOverwrite(), finalPath, temporaryPath, &uploadError)) {
                    return SendFileTransferError(socket);
                }
                state->BeginFileUpload(finalPath, temporaryPath);
            }
            return SendFileTransferPacketMessage(socket, rfbFileAcceptHeader, 0, 1, std::vector<CARD8>());
        case rfbFilePacket:
            if (!IsWriteFileTransferMode(mode) || !state->FileUploadActive()) {
                if (stats) {
                    stats->fileTransferBytesDiscarded += decision.payloadBytes;
                }
                return SendFileTransferError(socket);
            }
            if (!AppendUploadPayload(state->FileUploadPath(), payload)) {
                state->EndFileUpload();
                return SendFileTransferError(socket);
            }
            state->AddFileUploadBytes(static_cast<CARD32>(payload.size()));
            return true;
        case rfbEndOfFile:
            if (state->FileUploadActive()) {
                std::string commitError;
                const bool committed = CommitFileTransferUpload(state->FileUploadTemporaryPath(), state->FileUploadFinalPath(), &commitError);
                if (!committed) {
                    AbortFileTransferUpload(state->FileUploadTemporaryPath());
                    state->EndFileUpload();
                    return SendFileTransferError(socket);
                }
                state->EndFileUpload();
            }
            return true;
        case rfbCommand:
            if (!IsWriteFileTransferMode(mode)) {
                return SendFileTransferAccess(socket, false);
            }
            return SendFileTransferCommandResult(socket, ExecuteFileTransferCommand(state->FileTransferRoot(), message.contentParam, requestedPath, mode));
        default:
            if (stats) {
                stats->fileTransferBytesDiscarded += decision.payloadBytes;
            }
            return SendFileTransferAbort(socket, decision.abortReason);
        }
    }
    default:
        return false;
    }
}

bool RfbServerSession::ServeUntilFramebufferUpdate(RfbTransport& socket, const Framebuffer& framebuffer, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
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

bool RfbServerSession::ServeFramebufferUpdates(RfbTransport& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
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

bool RfbServerSession::SendBell(RfbTransport& socket) const
{
    const std::vector<CARD8> bytes = EncodeBell();
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool RfbServerSession::SendServerCutText(RfbTransport& socket, const std::string& text) const
{
    const std::vector<CARD8> bytes = EncodeServerCutText(text);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool RfbServerSession::SendCursorShape(RfbTransport& socket, RfbClientState& state) const
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

bool RfbServerSession::SendFileTransferAbort(RfbTransport& socket, CARD16 contentParam, CARD32 size) const
{
    const std::vector<CARD8> bytes = EncodeFileTransferAbort(contentParam, size);
    return socket.WriteAll(bytes.data(), bytes.size());
}

bool RfbServerSession::RunHandshake(TcpSocket& socket, const ServerConfig& config, RfbClientState *state) const
{
    std::unique_ptr<RfbTransport> transport;
    return RunHandshake(socket, config, state, transport);
}

bool RfbServerSession::RunHandshake(TcpSocket& socket, const ServerConfig& config, RfbClientState *state, std::unique_ptr<RfbTransport>& transport) const
{
    transport.reset();
    std::string error;
    if (!config.Validate(&error)) {
        return false;
    }

    std::unique_ptr<RfbTransport> clear(new TcpRfbTransport(socket));
    if (config.TransportSecurity() == TransportSecurityMode::None) {
        if (!RunHandshake(*clear, config, state)) {
            return false;
        }
        transport = std::move(clear);
        return true;
    }

    const std::string version = ProtocolVersion38();
    if (!clear->WriteAll(version.data(), version.size())) {
        return false;
    }
    char clientVersion[sz_rfbProtocolVersionMsg] = {};
    if (!clear->ReadExact(clientVersion, sizeof(clientVersion)) ||
        !IsProtocolVersionMessage(std::string(clientVersion, sizeof(clientVersion)))) {
        return false;
    }

    const std::vector<CARD8> security = SecurityTypesForConfig(config);
    if (!clear->WriteAll(security.data(), security.size())) {
        return false;
    }
    CARD8 selectedSecurity = 0;
    if (!clear->ReadExact(&selectedSecurity, sizeof(selectedSecurity)) || selectedSecurity != rfbVeNCypt) {
        return false;
    }
    if (!RunVeNCryptX509VncNegotiation(*clear)) {
        return false;
    }

    std::unique_ptr<RfbTransport> tls;
    if (!CreateOpenSslServerTransport(socket, config.TlsCertificateFile(), config.TlsPrivateKeyFile(), tls, &error)) {
        std::cerr << "TLS transport failed: " << error << "\n";
        return false;
    }
    if (!RunVncPasswordSecurity(*tls, config) || !RunClientInitAndServerInit(*tls, config, state)) {
        std::cerr << "TLS RFB authentication/client-init failed\n";
        return false;
    }
    transport = std::move(tls);
    return true;
}

bool RfbServerSession::ServeFramebufferUpdateRequest(TcpSocket& socket, const Framebuffer& framebuffer) const
{
    TcpRfbTransport transport(socket);
    return ServeFramebufferUpdateRequest(transport, framebuffer);
}

bool RfbServerSession::ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    TcpRfbTransport transport(socket);
    return ServeNextClientMessage(transport, framebuffer, updateSent, stats, state, inputSink, forceRawIncremental, clipboardSink, clipboardSource);
}

bool RfbServerSession::ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    TcpRfbTransport transport(socket);
    return ServeUntilFramebufferUpdate(transport, framebuffer, maxMessages, stats, state, inputSink, forceRawIncremental, clipboardSink, clipboardSource);
}

bool RfbServerSession::ServeFramebufferUpdates(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages, RfbSessionStats *stats, RfbClientState *state, RfbInputSink *inputSink, bool forceRawIncremental, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource) const
{
    TcpRfbTransport transport(socket);
    return ServeFramebufferUpdates(transport, framebuffer, updateCount, maxMessages, stats, state, inputSink, forceRawIncremental, clipboardSink, clipboardSource);
}

bool RfbServerSession::SendBell(TcpSocket& socket) const
{
    TcpRfbTransport transport(socket);
    return SendBell(transport);
}

bool RfbServerSession::SendServerCutText(TcpSocket& socket, const std::string& text) const
{
    TcpRfbTransport transport(socket);
    return SendServerCutText(transport, text);
}

bool RfbServerSession::SendCursorShape(TcpSocket& socket, RfbClientState& state) const
{
    TcpRfbTransport transport(socket);
    return SendCursorShape(transport, state);
}

bool RfbServerSession::SendFileTransferAbort(TcpSocket& socket, CARD16 contentParam, CARD32 size) const
{
    TcpRfbTransport transport(socket);
    return SendFileTransferAbort(transport, contentParam, size);
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
