// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbSession.h"

#include "vncPortableRfb.h"

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

} // namespace portable
} // namespace winvnc
} // namespace uvnc
