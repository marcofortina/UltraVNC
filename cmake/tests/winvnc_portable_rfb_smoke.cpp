// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfb.h"
#include "vncPortableServerConfig.h"

#include <cstring>
#include <iostream>

using namespace uvnc::winvnc::portable;

int main()
{
    if (ProtocolVersion38() != "RFB 003.008\n") {
        std::cerr << "unexpected protocol version\n";
        return 1;
    }
    if (!IsProtocolVersionMessage(ProtocolVersion38()) || IsProtocolVersionMessage("RFB 3.8\n")) {
        std::cerr << "protocol version validation failed\n";
        return 1;
    }
    const std::vector<CARD8> security = NoAuthSecurityTypes();
    if (security.size() != 2 || security[0] != 1 || security[1] != rfbNoAuth) {
        std::cerr << "unexpected security types\n";
        return 1;
    }
    const std::string name = "memory";
    const std::vector<CARD8> init = ServerInitBytes(640, 480, ServerConfig::DefaultPixelFormat(), name);
    if (init.size() != sz_rfbServerInitMsg + name.size()) {
        std::cerr << "server init size mismatch\n";
        return 1;
    }
    rfbServerInitMsg msg;
    std::memcpy(&msg, init.data(), sz_rfbServerInitMsg);
    if (Swap16IfLE(msg.framebufferWidth) != 640 || Swap16IfLE(msg.framebufferHeight) != 480) {
        std::cerr << "server init dimensions mismatch\n";
        return 1;
    }
    if (Swap32IfLE(msg.nameLength) != name.size()) {
        std::cerr << "server init name length mismatch\n";
        return 1;
    }
    return 0;
}
