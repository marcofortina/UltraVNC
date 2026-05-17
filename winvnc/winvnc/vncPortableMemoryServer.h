// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_MEMORY_SERVER_H
#define UVNC_WINVNC_PORTABLE_MEMORY_SERVER_H

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbSession.h"
#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class MemoryServer {
public:
    MemoryServer();

    bool Start(const ServerConfig& config);
    bool ServeOne();
    bool ServeOneUpdate();
    void Stop();
    bool Running() const { return listener_.Valid(); }
    unsigned short Port() const { return listener_.Port(); }

private:
    ServerConfig config_;
    TcpListener listener_;
    Framebuffer framebuffer_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_MEMORY_SERVER_H
