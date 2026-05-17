// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"

namespace uvnc {
namespace winvnc {
namespace portable {

MemoryServer::MemoryServer()
    : config_()
{
}

bool MemoryServer::Start(const ServerConfig& config)
{
    std::string error;
    if (!config.Validate(&error)) {
        return false;
    }
    config_ = config;
    return listener_.Listen(config_.BindAddress(), config_.Port());
}

bool MemoryServer::ServeOne()
{
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.Accept(client)) {
        return false;
    }
    return RfbServerSession().RunHandshake(client, config_);
}

void MemoryServer::Stop()
{
    listener_.Close();
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
