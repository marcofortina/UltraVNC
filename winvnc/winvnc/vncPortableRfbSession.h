// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_SESSION_H
#define UVNC_WINVNC_PORTABLE_RFB_SESSION_H

#include "vncPortableDesktopSource.h"
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
    unsigned int pointerPositionUpdatesSent;
    unsigned int fileTransferMessages;
    unsigned int fileTransferBytesDiscarded;

    RfbSessionStats();
};

class RfbInputSink {
public:
    virtual ~RfbInputSink() {}
    virtual bool InjectKey(const KeyEvent& event, std::string *error = nullptr) = 0;
    virtual bool InjectPointer(const PointerEvent& event, std::string *error = nullptr) = 0;
};

class RfbClipboardSink {
public:
    virtual ~RfbClipboardSink() {}
    virtual bool SetText(const std::string& text, std::string *error = nullptr) = 0;
};

class RfbClipboardSource {
public:
    virtual ~RfbClipboardSource() {}
    virtual bool GetText(std::string& text, std::string *error = nullptr) const = 0;
};

class RfbServerSession {
public:
    bool RunHandshake(TcpSocket& socket, const ServerConfig& config, RfbClientState *state = nullptr) const;
    bool ServeFramebufferUpdateRequest(TcpSocket& socket, const Framebuffer& framebuffer) const;
    bool ServeNextClientMessage(TcpSocket& socket, const Framebuffer& framebuffer, bool& updateSent, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr, bool forceRawIncremental = false, RfbClipboardSink *clipboardSink = nullptr, RfbClipboardSource *clipboardSource = nullptr) const;
    bool ServeUntilFramebufferUpdate(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int maxMessages = 32, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr, bool forceRawIncremental = false, RfbClipboardSink *clipboardSink = nullptr, RfbClipboardSource *clipboardSource = nullptr) const;
    bool ServeFramebufferUpdates(TcpSocket& socket, const Framebuffer& framebuffer, unsigned int updateCount, unsigned int maxMessages = 128, RfbSessionStats *stats = nullptr, RfbClientState *state = nullptr, RfbInputSink *inputSink = nullptr, bool forceRawIncremental = false, RfbClipboardSink *clipboardSink = nullptr, RfbClipboardSource *clipboardSource = nullptr) const;
    bool SendBell(TcpSocket& socket) const;
    bool SendServerCutText(TcpSocket& socket, const std::string& text) const;
    bool SendCursorShape(TcpSocket& socket, RfbClientState& state) const;
    bool SendFileTransferAbort(TcpSocket& socket, CARD16 contentParam = 0, CARD32 size = 0) const;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_SESSION_H
