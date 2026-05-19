// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExtendedClipboard.h"

#include <algorithm>
#include <cstring>
#include <limits>

#include <zlib.h>

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

CARD32 ReadU32(const CARD8 *data)
{
    CARD32 value = 0;
    std::memcpy(&value, data, sizeof(value));
    return Swap32IfLE(value);
}

void AppendU32(std::vector<CARD8>& out, CARD32 value)
{
    const CARD32 wire = Swap32IfLE(value);
    const CARD8 *bytes = reinterpret_cast<const CARD8 *>(&wire);
    out.insert(out.end(), bytes, bytes + sizeof(wire));
}

bool CompressZlib(const std::vector<CARD8>& input, std::vector<CARD8>& output)
{
    uLongf bound = compressBound(static_cast<uLong>(input.size()));
    output.resize(bound);
    const int result = compress2(output.data(), &bound, input.data(), static_cast<uLong>(input.size()), Z_BEST_COMPRESSION);
    if (result != Z_OK) {
        output.clear();
        return false;
    }
    output.resize(bound);
    return true;
}

bool DecompressZlib(const CARD8 *data, std::size_t size, std::vector<CARD8>& output, std::size_t limit)
{
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }

    stream.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(data));
    stream.avail_in = static_cast<uInt>(size);

    bool ok = false;
    CARD8 buffer[4096];
    while (true) {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        const int result = inflate(&stream, Z_NO_FLUSH);
        if (result != Z_OK && result != Z_STREAM_END) {
            break;
        }
        const std::size_t produced = sizeof(buffer) - stream.avail_out;
        if (output.size() + produced > limit) {
            break;
        }
        output.insert(output.end(), buffer, buffer + produced);
        if (result == Z_STREAM_END) {
            ok = stream.avail_in == 0;
            break;
        }
        if (produced == 0 && stream.avail_in == 0) {
            break;
        }
    }
    inflateEnd(&stream);
    return ok;
}

std::vector<CARD8> EncodeExtendedPayload(CARD32 flags, const std::vector<CARD8>& body)
{
    std::vector<CARD8> payload;
    AppendU32(payload, flags);
    payload.insert(payload.end(), body.begin(), body.end());
    return payload;
}

std::vector<CARD8> EncodeCutTextMessage(CARD8 type, const std::vector<CARD8>& payload)
{
    rfbServerCutTextMsg header;
    std::memset(&header, 0, sizeof(header));
    header.type = type;
    const int signedLength = -static_cast<int>(payload.size());
    header.length = Swap32IfLE(static_cast<CARD32>(signedLength));

    std::vector<CARD8> bytes(sz_rfbServerCutTextMsg + payload.size());
    std::memcpy(bytes.data(), &header, sz_rfbServerCutTextMsg);
    if (!payload.empty()) {
        std::memcpy(bytes.data() + sz_rfbServerCutTextMsg, payload.data(), payload.size());
    }
    return bytes;
}

} // namespace

ExtendedClipboardPayload::ExtendedClipboardPayload()
    : flags(0), text(), textPresent(false), malformed(false)
{
}

bool IsExtendedClipboardWireLength(CARD32 wireLength)
{
    return static_cast<int32_t>(Swap32IfLE(wireLength)) < 0;
}

unsigned int ExtendedClipboardPayloadLength(CARD32 wireLength)
{
    const int32_t length = static_cast<int32_t>(Swap32IfLE(wireLength));
    if (length >= 0 || length == std::numeric_limits<int32_t>::min()) {
        return 0;
    }
    return static_cast<unsigned int>(-length);
}

std::vector<CARD8> EncodeExtendedClipboardCaps(CARD32 caps, CARD32 textLimit)
{
    std::vector<CARD8> body;
    if (caps & clipText) {
        AppendU32(body, textLimit);
    }
    return EncodeExtendedPayload(caps, body);
}

std::vector<CARD8> EncodeExtendedClipboardNotify(bool textAvailable)
{
    return EncodeExtendedPayload(clipNotify | (textAvailable ? clipText : 0), std::vector<CARD8>());
}

std::vector<CARD8> EncodeExtendedClipboardRequest(CARD32 formats)
{
    return EncodeExtendedPayload(clipRequest | (formats & clipFormatMask), std::vector<CARD8>());
}

std::vector<CARD8> EncodeExtendedClipboardPeek(CARD32 formats)
{
    return EncodeExtendedPayload(clipPeek | (formats & clipFormatMask), std::vector<CARD8>());
}

std::vector<CARD8> EncodeExtendedClipboardProvideText(const std::string& text)
{
    std::vector<CARD8> uncompressed;
    AppendU32(uncompressed, static_cast<CARD32>(text.size()));
    uncompressed.insert(uncompressed.end(), text.begin(), text.end());
    std::vector<CARD8> compressed;
    if (!CompressZlib(uncompressed, compressed)) {
        compressed.clear();
    }
    return EncodeExtendedPayload(clipProvide | clipText, compressed);
}

bool DecodeExtendedClipboardPayload(const std::vector<CARD8>& payload, ExtendedClipboardPayload& out)
{
    out = ExtendedClipboardPayload();
    if (payload.size() < sz_rfbExtendedClipboardData) {
        out.malformed = true;
        return false;
    }
    out.flags = ReadU32(payload.data());
    const CARD32 action = out.flags & clipActionMask;
    if (action == clipProvide) {
        return DecodeExtendedClipboardProvidedText(payload, out.text) ? (out.textPresent = true, true) : (out.malformed = true, false);
    }
    return true;
}

bool DecodeExtendedClipboardProvidedText(const std::vector<CARD8>& payload, std::string& text)
{
    text.clear();
    if (payload.size() < sz_rfbExtendedClipboardData) {
        return false;
    }
    const CARD32 flags = ReadU32(payload.data());
    if ((flags & clipActionMask) != clipProvide || (flags & clipText) == 0) {
        return false;
    }
    std::vector<CARD8> uncompressed;
    if (!DecompressZlib(payload.data() + sz_rfbExtendedClipboardData,
                        payload.size() - sz_rfbExtendedClipboardData,
                        uncompressed,
                        kExtendedClipboardDefaultTextLimit + sizeof(CARD32))) {
        return false;
    }
    if (uncompressed.size() < sizeof(CARD32)) {
        return false;
    }
    const CARD32 textLength = ReadU32(uncompressed.data());
    if (textLength > kExtendedClipboardDefaultTextLimit || uncompressed.size() < sizeof(CARD32) + textLength) {
        return false;
    }
    text.assign(reinterpret_cast<const char *>(uncompressed.data() + sizeof(CARD32)), textLength);
    return true;
}

std::vector<CARD8> EncodeExtendedServerCutText(const std::vector<CARD8>& extendedPayload)
{
    return EncodeCutTextMessage(rfbServerCutText, extendedPayload);
}

std::vector<CARD8> EncodeExtendedClientCutText(const std::vector<CARD8>& extendedPayload)
{
    return EncodeCutTextMessage(rfbClientCutText, extendedPayload);
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
