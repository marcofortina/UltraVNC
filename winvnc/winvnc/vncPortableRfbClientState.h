// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_CLIENT_STATE_H
#define UVNC_WINVNC_PORTABLE_RFB_CLIENT_STATE_H

#include "vncPortableRfbMessages.h"
#include "vncPortableServerConfig.h"

#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

class RfbClientState {
public:
    explicit RfbClientState(const ServerConfig& config = ServerConfig());

    const rfbPixelFormat& PixelFormat() const { return pixelFormat_; }
    const std::vector<CARD32>& Encodings() const { return encodings_; }
    const KeyEvent& LastKeyEvent() const { return lastKeyEvent_; }
    const PointerEvent& LastPointerEvent() const { return lastPointerEvent_; }
    unsigned int KeyEventCount() const { return keyEventCount_; }
    unsigned int PointerEventCount() const { return pointerEventCount_; }
    unsigned int ClientCutTextMessages() const { return clientCutTextMessages_; }
    unsigned int ClientCutTextBytes() const { return clientCutTextBytes_; }

    void SetPixelFormat(const rfbPixelFormat& format);
    void SetEncodings(const std::vector<CARD32>& encodings);
    void RecordKeyEvent(const KeyEvent& event);
    void RecordPointerEvent(const PointerEvent& event);
    void RecordClientCutText(unsigned int bytes);

private:
    rfbPixelFormat pixelFormat_;
    std::vector<CARD32> encodings_;
    KeyEvent lastKeyEvent_;
    PointerEvent lastPointerEvent_;
    unsigned int keyEventCount_;
    unsigned int pointerEventCount_;
    unsigned int clientCutTextMessages_;
    unsigned int clientCutTextBytes_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_CLIENT_STATE_H
