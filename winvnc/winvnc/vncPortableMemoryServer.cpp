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

namespace {

bool SendInitialServerMessages(RfbServerSession& session, TcpSocket& client, const ServerConfig& config)
{
    if (config.BellOnConnect() && !session.SendBell(client)) {
        return false;
    }
    if (!config.ServerCutText().empty() && !session.SendServerCutText(client, config.ServerCutText())) {
        return false;
    }
    return true;
}

} // namespace

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
    RfbServerSession session;
    return session.RunHandshake(client, config_) &&
           SendInitialServerMessages(session, client, config_);
}

bool MemoryServer::ServeOneUpdate(RfbInputSink *inputSink)
{
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.Accept(client)) {
        return false;
    }
    RfbServerSession session;
    RfbClientState state(config_);
    return session.RunHandshake(client, config_) &&
           SendInitialServerMessages(session, client, config_) &&
           session.ServeUntilFramebufferUpdate(client, framebuffer_, 32, nullptr, &state, inputSink);
}

bool MemoryServer::ServeOneUpdates(unsigned int updateCount, RfbInputSink *inputSink)
{
    bool accepted = false;
    return TryServeOneUpdates(updateCount, inputSink, 0, accepted) && accepted;
}

bool MemoryServer::TryServeOneUpdates(unsigned int updateCount, RfbInputSink *inputSink, unsigned int acceptTimeoutMs, bool& accepted)
{
    accepted = false;
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.AcceptWithTimeoutMs(client, acceptTimeoutMs)) {
        return true;
    }
    accepted = true;
    RfbServerSession session;
    RfbClientState state(config_);
    return session.RunHandshake(client, config_) &&
           SendInitialServerMessages(session, client, config_) &&
           session.ServeFramebufferUpdates(client, framebuffer_, updateCount, 128, nullptr, &state, inputSink);
}

bool MemoryServer::ServeOneUpdatesFromSource(DesktopSource& source, unsigned int updateCount, RfbInputSink *inputSink, unsigned int maxMessages)
{
    bool accepted = false;
    return TryServeOneUpdatesFromSource(source, updateCount, inputSink, maxMessages, 0, accepted) && accepted;
}

bool MemoryServer::TryServeOneUpdatesFromSource(DesktopSource& source, unsigned int updateCount, RfbInputSink *inputSink, unsigned int maxMessages, unsigned int acceptTimeoutMs, bool& accepted)
{
    accepted = false;
    if (!listener_.Valid()) {
        return false;
    }
    TcpSocket client;
    if (!listener_.AcceptWithTimeoutMs(client, acceptTimeoutMs)) {
        return true;
    }
    accepted = true;

    RfbServerSession session;
    if (!session.RunHandshake(client, config_) ||
        !SendInitialServerMessages(session, client, config_)) {
        return false;
    }

    RfbSessionStats stats;
    RfbClientState state(config_);
    unsigned int sent = 0;
    for (unsigned int i = 0; i < maxMessages && sent < updateCount; ++i) {
        Framebuffer current;
        rfb::Region2D changed;
        if (!source.Snapshot(current, changed) ||
            current.Width() != config_.Width() ||
            current.Height() != config_.Height()) {
            return false;
        }

        bool updateSent = false;
        if (!session.ServeNextClientMessage(client, current, updateSent, &stats, &state, inputSink, true)) {
            return false;
        }
        if (updateSent) {
            sent += 1;
        }
    }
    return sent == updateCount;
}

void MemoryServer::Stop()
{
    listener_.Close();
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
