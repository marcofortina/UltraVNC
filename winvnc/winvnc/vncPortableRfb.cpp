// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfb.h"

#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

std::string ProtocolVersion38()
{
    return "RFB 003.008\n";
}

bool IsProtocolVersionMessage(const std::string& value)
{
    return value.size() == sz_rfbProtocolVersionMsg &&
           value.compare(0, 4, "RFB ") == 0 &&
           value[7] == '.' &&
           value[11] == '\n';
}

std::vector<CARD8> SecurityTypesForConfig(const ServerConfig& config)
{
    if (config.TransportSecurity() == TransportSecurityMode::VeNCryptX509Vnc) {
        std::vector<CARD8> types;
        types.push_back(1);
        types.push_back(rfbVeNCypt);
        return types;
    }
    return SecurityTypesForAuthMode(config.AuthMode());
}

std::vector<CARD8> SecurityTypesForAuthMode(ServerAuthMode mode)
{
    std::vector<CARD8> types;
    types.push_back(1);
    types.push_back(mode == ServerAuthMode::VncPassword ? rfbVncAuth : rfbNoAuth);
    return types;
}

std::vector<CARD8> NoAuthSecurityTypes()
{
    return SecurityTypesForAuthMode(ServerAuthMode::NoAuth);
}

CARD32 AuthOkValue()
{
    return Swap32IfLE(rfbVncAuthOK);
}

CARD32 AuthFailedValue()
{
    return Swap32IfLE(rfbVncAuthFailed);
}

rfbPixelFormat NetworkPixelFormat(const rfbPixelFormat& format)
{
    rfbPixelFormat out = format;
    out.redMax = Swap16IfLE(out.redMax);
    out.greenMax = Swap16IfLE(out.greenMax);
    out.blueMax = Swap16IfLE(out.blueMax);
    return out;
}

std::vector<CARD8> ServerInitBytes(unsigned int width,
                                   unsigned int height,
                                   const rfbPixelFormat& format,
                                   const std::string& name)
{
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(width));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(height));
    init.format = NetworkPixelFormat(format);
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));

    std::vector<CARD8> bytes(sz_rfbServerInitMsg + name.size());
    std::memcpy(bytes.data(), &init, sz_rfbServerInitMsg);
    if (!name.empty()) {
        std::memcpy(bytes.data() + sz_rfbServerInitMsg, name.data(), name.size());
    }
    return bytes;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
