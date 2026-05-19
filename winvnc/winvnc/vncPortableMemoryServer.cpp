// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableFramebufferPattern.h"

#include <chrono>
#include <thread>

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

bool SendInitialServerMessages(RfbServerSession& session, RfbTransport& client, const ServerConfig& config, RfbClipboardSource *clipboardSource = nullptr, RfbClientState *state = nullptr)
{
    if (config.BellOnConnect() && !session.SendBell(client)) {
        return false;
    }
    if (!config.ServerCutText().empty() && !session.SendServerCutText(client, config.ServerCutText())) {
        return false;
    }
    if (clipboardSource) {
        std::string text;
        std::string error;
        if (!clipboardSource->GetText(text, &error)) {
            return false;
        }
        if (!text.empty() && !session.SendServerCutText(client, text)) {
            return false;
        }
        if (!text.empty() && state) {
            state->RecordServerCutTextSent(text);
        }
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
    RfbClientState state(config_);
    std::unique_ptr<RfbTransport> transport;
    return session.RunHandshake(client, config_, &state, transport) &&
           SendInitialServerMessages(session, *transport, config_, nullptr, &state);
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
    std::unique_ptr<RfbTransport> transport;
    return session.RunHandshake(client, config_, &state, transport) &&
           SendInitialServerMessages(session, *transport, config_, nullptr, &state) &&
           session.ServeUntilFramebufferUpdate(*transport, framebuffer_, 32, nullptr, &state, inputSink);
}

bool MemoryServer::ServeOneUpdates(unsigned int updateCount, RfbInputSink *inputSink, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    bool accepted = false;
    return TryServeOneUpdates(updateCount, inputSink, 0, accepted, clipboardSink, clipboardSource, clientPolicy, cursorSource) && accepted;
}

bool MemoryServer::TryAccept(TcpSocket& client, unsigned int acceptTimeoutMs, bool& accepted)
{
    accepted = false;
    if (!listener_.Valid()) {
        return false;
    }
    if (!listener_.AcceptWithTimeoutMs(client, acceptTimeoutMs)) {
        return true;
    }
    accepted = true;
    return true;
}

bool MemoryServer::ServeConnectedUpdates(TcpSocket client, unsigned int updateCount, RfbInputSink *inputSink, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    if (!client.Valid()) {
        return false;
    }
    RfbServerSession session;
    RfbClientState state(config_);
    std::unique_ptr<RfbTransport> transport;
    if (!session.RunHandshake(client, config_, &state, transport)) {
        return false;
    }
    ClientConnectionLease lease(clientPolicy, state.SharedClientRequested());
    if (clientPolicy && !lease.Acquire()) {
        return false;
    }
    return SendInitialServerMessages(session, *transport, config_, clipboardSource, &state) &&
        session.ServeFramebufferUpdates(*transport, framebuffer_, updateCount, 128, nullptr, &state, inputSink, false, clipboardSink, clipboardSource, cursorSource);
}

bool MemoryServer::TryServeOneUpdates(unsigned int updateCount, RfbInputSink *inputSink, unsigned int acceptTimeoutMs, bool& accepted, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    TcpSocket client;
    if (!TryAccept(client, acceptTimeoutMs, accepted) || !accepted) {
        return !accepted;
    }
    return ServeConnectedUpdates(std::move(client), updateCount, inputSink, clipboardSink, clipboardSource, clientPolicy, cursorSource);
}

bool MemoryServer::ServeOneUpdatesFromSource(DesktopSource& source, unsigned int updateCount, RfbInputSink *inputSink, unsigned int maxMessages, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    bool accepted = false;
    return TryServeOneUpdatesFromSource(source, updateCount, inputSink, maxMessages, 0, accepted, clipboardSink, clipboardSource, clientPolicy, cursorSource) && accepted;
}

bool MemoryServer::ServeConnectedUpdatesFromSource(TcpSocket client, DesktopSource& source, unsigned int updateCount, RfbInputSink *inputSink, unsigned int maxMessages, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    if (!client.Valid()) {
        return false;
    }
    RfbServerSession session;
    RfbClientState state(config_);
    std::unique_ptr<RfbTransport> transport;
    if (!session.RunHandshake(client, config_, &state, transport)) {
        return false;
    }
    ClientConnectionLease lease(clientPolicy, state.SharedClientRequested());
    if (clientPolicy && !lease.Acquire()) {
        return false;
    }
    if (!SendInitialServerMessages(session, *transport, config_, clipboardSource, &state)) {
        return false;
    }

    RfbSessionStats stats;
    unsigned int sent = 0;
    for (unsigned int i = 0; i < maxMessages && sent < updateCount; ++i) {
        Framebuffer current;
        rfb::Region2D changed;
        if (!source.Snapshot(current, changed)) {
            return false;
        }

        bool updateSent = false;
        if (!session.ServeNextClientMessage(*transport, current, updateSent, &stats, &state, inputSink, false, clipboardSink, clipboardSource, cursorSource, &changed)) {
            return false;
        }
        if (updateSent) {
            sent += 1;
            if (config_.UpdatePacingMs() > 0 && sent < updateCount) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config_.UpdatePacingMs()));
            }
        }
    }
    return sent == updateCount;
}

bool MemoryServer::TryServeOneUpdatesFromSource(DesktopSource& source, unsigned int updateCount, RfbInputSink *inputSink, unsigned int maxMessages, unsigned int acceptTimeoutMs, bool& accepted, RfbClipboardSink *clipboardSink, RfbClipboardSource *clipboardSource, ClientConnectionPolicy *clientPolicy, RfbCursorSource *cursorSource)
{
    TcpSocket client;
    if (!TryAccept(client, acceptTimeoutMs, accepted) || !accepted) {
        return !accepted;
    }
    return ServeConnectedUpdatesFromSource(std::move(client), source, updateCount, inputSink, maxMessages, clipboardSink, clipboardSource, clientPolicy, cursorSource);
}

void MemoryServer::Stop()
{
    listener_.Close();
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
