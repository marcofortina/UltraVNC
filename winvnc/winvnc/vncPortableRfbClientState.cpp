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
      clientCutTextBytes_(0)
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

void RfbClientState::SetPixelFormat(const rfbPixelFormat& format)
{
    pixelFormat_ = format;
}

void RfbClientState::SetEncodings(const std::vector<CARD32>& encodings)
{
    encodings_ = encodings;
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
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
