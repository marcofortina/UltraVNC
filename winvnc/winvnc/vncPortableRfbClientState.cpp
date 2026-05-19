// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbClientState.h"

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

RfbClientState::RfbClientState(const ServerConfig& config)
    : pixelFormat_(config.PixelFormat()),
      encodings_(1, rfbEncodingRaw),
      lastKeyEvent_(),
      lastPointerEvent_(),
      keyEventCount_(0),
      pointerEventCount_(0),
      clientCutTextMessages_(0),
      clientCutTextBytes_(0),
      lastClientCutText_(),
      lastServerCutText_(),
      cursorShapeSent_(false),
      sharedClientRequested_(true),
      clientInitReceived_(false),
      fileTransferMode_(config.FileTransferModeValue()),
      fileTransferPayloadLimit_(config.FileTransferPayloadLimit())
{
    std::memset(&lastKeyEvent_, 0, sizeof(lastKeyEvent_));
    std::memset(&lastPointerEvent_, 0, sizeof(lastPointerEvent_));
}

bool RfbClientState::SupportsEncoding(CARD32 encoding) const
{
    return std::find(encodings_.begin(), encodings_.end(), encoding) != encodings_.end();
}

bool RfbClientState::SupportsPointerPositionUpdates() const
{
    return SupportsEncoding(rfbEncodingPointerPos);
}

bool RfbClientState::SupportsRichCursorUpdates() const
{
    return SupportsEncoding(rfbEncodingRichCursor);
}

bool RfbClientState::SupportsXCursorUpdates() const
{
    return SupportsEncoding(rfbEncodingXCursor);
}

bool RfbClientState::SupportsCursorShapeUpdates() const
{
    return SupportsRichCursorUpdates() || SupportsXCursorUpdates();
}

void RfbClientState::SetPixelFormat(const rfbPixelFormat& format)
{
    pixelFormat_ = format;
}

void RfbClientState::SetEncodings(const std::vector<CARD32>& encodings)
{
    encodings_ = encodings;
    cursorShapeSent_ = false;
}

void RfbClientState::MarkCursorShapeSent()
{
    cursorShapeSent_ = true;
}

void RfbClientState::RecordClientInit(bool shared)
{
    sharedClientRequested_ = shared;
    clientInitReceived_ = true;
}

void RfbClientState::RecordKeyEvent(const KeyEvent& event)
{
    lastKeyEvent_ = event;
    keyEventCount_ += 1;
}

void RfbClientState::RecordPointerEvent(const PointerEvent& event)
{
    lastPointerEvent_ = event;
    pointerEventCount_ += 1;
}

void RfbClientState::RecordClientCutText(unsigned int bytes)
{
    clientCutTextMessages_ += 1;
    clientCutTextBytes_ += bytes;
    lastClientCutText_.clear();
}

void RfbClientState::RecordClientCutText(const std::string& text)
{
    clientCutTextMessages_ += 1;
    clientCutTextBytes_ += static_cast<unsigned int>(text.size());
    lastClientCutText_ = text;
}

void RfbClientState::RecordServerCutTextSent(const std::string& text)
{
    lastServerCutText_ = text;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
