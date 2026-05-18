// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_SESSION_H
#define UVNC_WINVNC_PORTABLE_RFB_SESSION_H

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbClientState.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"

namespace uvnc {
namespace winvnc {
namespace portable {

struct RfbSessionStats {
    unsigned int messagesProcessed;
    unsigned int framebufferUpdatesSent;
    unsigned int setPixelFormatMessages;
    unsigned int setEncodingsMessages;
    unsigned int keyEvents;
    unsigned int pointerEvents;
    unsigned int clientCutTextMessages;

    RfbSessionStats();
};

class RfbInputSink {
public:
    virtual ~RfbInputSink() {}
    virtual bool InjectKey(const KeyEvent& event, std::string *error = nullptr) = 0;
    virtual bool InjectPointer(const PointerEvent& event, std::string *error = nullptr) = 0;
};

class RfbServerSession {
public:
    bool RunHandshake(TcpSocket& socket, const ServerConfig& config) const;
    bool ServeFramebufferUpdateRequest(TcpSocket& socket, const Framebuffer& framebuffer) const;
    bool ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr) const;
    bool ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages = 32, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr) const;
    bool ServeFramebufferUpdates(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages = 128, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr) const;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_SESSION_H
