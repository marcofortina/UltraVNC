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

bool RfbServerSession::ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent) const
{
    updateSent = false;
    CARD8 type = 0;
    if (!socket.ReadExact(&type, sizeof(type))) {
        return false;
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
        const std::vector<CARD8> update = RawFramebufferUpdateBytes(framebuffer, request);
        updateSent = socket.WriteAll(update.data(), update.size());
        return updateSent;
    }
    case rfbSetPixelFormat: {
        rfbSetPixelFormatMsg wire;
        wire.type = type;
        return socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbSetPixelFormatMsg - 1);
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
        return payload.empty() || socket.ReadExact(payload.data(), payload.size());
    }
    case rfbKeyEvent: {
        rfbKeyEventMsg wire;
        wire.type = type;
        return socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbKeyEventMsg - 1);
    }
    case rfbPointerEvent: {
        rfbPointerEventMsg wire;
        wire.type = type;
        return socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbPointerEventMsg - 1);
    }
    case rfbClientCutText: {
        rfbClientCutTextMsg wire;
        wire.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&wire) + 1, sz_rfbClientCutTextMsg - 1)) {
            return false;
        }
        const CARD32 length = Swap32IfLE(wire.length);
        std::vector<CARD8> payload(length);
        return payload.empty() || socket.ReadExact(payload.data(), payload.size());
    }
    default:
        return false;
    }
}

bool RfbServerSession::ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages) const
{
    for (unsigned int i = 0; i < maxMessages; ++i) {
        bool updateSent = false;
        if (!ServeNextClientMessage(socket, framebuffer, updateSent)) {
            return false;
        }
        if (updateSent) {
            return true;
        }
    }
    return false;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
