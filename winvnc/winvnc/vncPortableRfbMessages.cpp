// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"

#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

bool DecodeFramebufferUpdateRequest(const rfbFramebufferUpdateRequestMsg& message,
                                    FramebufferUpdateRequest& out)
{
    if (message.type != rfbFramebufferUpdateRequest) {
        return false;
    }
    out.incremental = message.incremental != 0;
    out.x = Swap16IfLE(message.x);
    out.y = Swap16IfLE(message.y);
    out.width = Swap16IfLE(message.w);
    out.height = Swap16IfLE(message.h);
    return true;
}

rfbFramebufferUpdateRequestMsg EncodeFramebufferUpdateRequest(const FramebufferUpdateRequest& request)
{
    rfbFramebufferUpdateRequestMsg message;
    message.type = rfbFramebufferUpdateRequest;
    message.incremental = request.incremental ? 1 : 0;
    message.x = Swap16IfLE(static_cast<CARD16>(request.x));
    message.y = Swap16IfLE(static_cast<CARD16>(request.y));
    message.w = Swap16IfLE(static_cast<CARD16>(request.width));
    message.h = Swap16IfLE(static_cast<CARD16>(request.height));
    return message;
}

bool DecodeKeyEvent(const rfbKeyEventMsg& message, KeyEvent& out)
{
    if (message.type != rfbKeyEvent) {
        return false;
    }
    out.down = message.down != 0;
    out.keysym = Swap32IfLE(message.key);
    return true;
}

rfbKeyEventMsg EncodeKeyEvent(const KeyEvent& event)
{
    rfbKeyEventMsg message;
    message.type = rfbKeyEvent;
    message.down = event.down ? 1 : 0;
    message.pad = 0;
    message.key = Swap32IfLE(event.keysym);
    return message;
}

bool DecodePointerEvent(const rfbPointerEventMsg& message, PointerEvent& out)
{
    if (message.type != rfbPointerEvent) {
        return false;
    }
    out.buttonMask = message.buttonMask;
    out.x = Swap16IfLE(message.x);
    out.y = Swap16IfLE(message.y);
    return true;
}

rfbPointerEventMsg EncodePointerEvent(const PointerEvent& event)
{
    rfbPointerEventMsg message;
    message.type = rfbPointerEvent;
    message.buttonMask = event.buttonMask;
    message.x = Swap16IfLE(static_cast<CARD16>(event.x));
    message.y = Swap16IfLE(static_cast<CARD16>(event.y));
    return message;
}

bool DecodeSetPixelFormat(const rfbSetPixelFormatMsg& message, rfbPixelFormat& out)
{
    if (message.type != rfbSetPixelFormat) {
        return false;
    }
    out = message.format;
    out.redMax = Swap16IfLE(out.redMax);
    out.greenMax = Swap16IfLE(out.greenMax);
    out.blueMax = Swap16IfLE(out.blueMax);
    return true;
}

rfbSetPixelFormatMsg EncodeSetPixelFormat(const rfbPixelFormat& format)
{
    rfbSetPixelFormatMsg message;
    message.type = rfbSetPixelFormat;
    message.pad1 = 0;
    message.pad2 = 0;
    message.format = format;
    message.format.redMax = Swap16IfLE(message.format.redMax);
    message.format.greenMax = Swap16IfLE(message.format.greenMax);
    message.format.blueMax = Swap16IfLE(message.format.blueMax);
    return message;
}

bool DecodeSetEncodingsHeader(const rfbSetEncodingsMsg& message, unsigned int& count)
{
    if (message.type != rfbSetEncodings) {
        return false;
    }
    count = Swap16IfLE(message.nEncodings);
    return true;
}

std::vector<CARD8> EncodeSetEncodings(const std::vector<CARD32>& encodings)
{
    rfbSetEncodingsMsg header;
    header.type = rfbSetEncodings;
    header.pad = 0;
    header.nEncodings = Swap16IfLE(static_cast<CARD16>(encodings.size()));

    std::vector<CARD8> bytes(sz_rfbSetEncodingsMsg + encodings.size() * sizeof(CARD32));
    std::memcpy(bytes.data(), &header, sz_rfbSetEncodingsMsg);
    CARD8 *out = bytes.data() + sz_rfbSetEncodingsMsg;
    for (std::size_t i = 0; i < encodings.size(); ++i) {
        const CARD32 wire = Swap32IfLE(encodings[i]);
        std::memcpy(out + i * sizeof(CARD32), &wire, sizeof(wire));
    }
    return bytes;
}

std::vector<CARD32> DecodeSetEncodingsPayload(const std::vector<CARD8>& payload)
{
    std::vector<CARD32> encodings;
    if (payload.size() % sizeof(CARD32) != 0) {
        return encodings;
    }
    encodings.resize(payload.size() / sizeof(CARD32));
    for (std::size_t i = 0; i < encodings.size(); ++i) {
        CARD32 wire = 0;
        std::memcpy(&wire, payload.data() + i * sizeof(CARD32), sizeof(wire));
        encodings[i] = Swap32IfLE(wire);
    }
    return encodings;
}


std::vector<CARD8> EncodeClientCutText(const std::string& text)
{
    rfbClientCutTextMsg header;
    header.type = rfbClientCutText;
    header.pad1 = 0;
    header.pad2 = 0;
    header.length = Swap32IfLE(static_cast<CARD32>(text.size()));

    std::vector<CARD8> bytes(sz_rfbClientCutTextMsg + text.size());
    std::memcpy(bytes.data(), &header, sz_rfbClientCutTextMsg);
    if (!text.empty()) {
        std::memcpy(bytes.data() + sz_rfbClientCutTextMsg, text.data(), text.size());
    }
    return bytes;
}

bool DecodeClientCutTextHeader(const rfbClientCutTextMsg& message, unsigned int& length)
{
    if (message.type != rfbClientCutText) {
        return false;
    }
    length = Swap32IfLE(message.length);
    return true;
}


std::vector<CARD8> EncodeServerCutText(const std::string& text)
{
    rfbServerCutTextMsg header;
    std::memset(&header, 0, sizeof(header));
    header.type = rfbServerCutText;
    header.length = Swap32IfLE(static_cast<CARD32>(text.size()));

    std::vector<CARD8> bytes(sz_rfbServerCutTextMsg + text.size());
    std::memcpy(bytes.data(), &header, sz_rfbServerCutTextMsg);
    if (!text.empty()) {
        std::memcpy(bytes.data() + sz_rfbServerCutTextMsg, text.data(), text.size());
    }
    return bytes;
}

std::vector<CARD8> EncodeBell()
{
    rfbBellMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbBell;
    std::vector<CARD8> bytes(sz_rfbBellMsg);
    std::memcpy(bytes.data(), &message, sz_rfbBellMsg);
    return bytes;
}

bool DecodeFileTransferHeader(const rfbFileTransferMsg& message, FileTransferMessage& out)
{
    if (message.type != rfbFileTransfer) {
        return false;
    }
    out.contentType = message.contentType;
    out.contentParam = Swap16IfLE(message.contentParam);
    out.size = Swap32IfLE(message.size);
    out.length = Swap32IfLE(message.length);
    return true;
}

std::vector<CARD8> EncodeFileTransferAbort(CARD16 contentParam, CARD32 size)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = rfbAbortFileTransfer;
    message.contentParam = Swap16IfLE(contentParam);
    message.size = Swap32IfLE(size);

    std::vector<CARD8> bytes(sz_rfbFileTransferMsg);
    std::memcpy(bytes.data(), &message, sz_rfbFileTransferMsg);
    return bytes;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
