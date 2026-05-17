// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableFramebufferPattern.h"

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
    framebuffer_.Reset(config_.Width(), config_.Height(), config_.PixelFormat());
    if (!ApplyFramebufferPattern(framebuffer_, config_.Pattern(), config_.FillByte())) {
        return false;
    }
    return listener_.Listen(config_.BindAddress(), config_.Port());
}

bool MemoryServer::StartWithFramebuffer(const ServerConfig& config, const Framebuffer& framebuffer)
{
    std::string error;
    if (!config.Validate(&error) || framebuffer.Empty() ||
        framebuffer.Width() != config.Width() ||
        framebuffer.Height() != config.Height() ||
        framebuffer.BytesPerPixel() != (config.PixelFormat().bitsPerPixel / 8)) {
        return false;
    }
    config_ = config;
    framebuffer_ = framebuffer;
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

bool MemoryServer::ServeOneUpdate()
{
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.Accept(client)) {
        return false;
    }
    RfbServerSession session;
    return session.RunHandshake(client, config_) &&
           session.ServeUntilFramebufferUpdate(client, framebuffer_);
}

bool MemoryServer::ServeOneUpdates(unsigned int updateCount)
{
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.Accept(client)) {
        return false;
    }
    RfbServerSession session;
    return session.RunHandshake(client, config_) &&
           session.ServeFramebufferUpdates(client, framebuffer_, updateCount);
}

void MemoryServer::Stop()
{
    listener_.Close();
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
