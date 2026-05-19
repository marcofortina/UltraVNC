// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
#define UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

struct FramebufferUpdateRequest {
    bool incremental;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

struct KeyEvent {
    bool down;
    CARD32 keysym;
};

struct PointerEvent {
    CARD8 buttonMask;
    unsigned int x;
    unsigned int y;
};

struct FileTransferMessage {
    CARD8 contentType;
    CARD16 contentParam;
    CARD32 size;
    CARD32 length;
};

bool DecodeFramebufferUpdateRequest(const rfbFramebufferUpdateRequestMsg& message,
                                    FramebufferUpdateRequest& out);
rfbFramebufferUpdateRequestMsg EncodeFramebufferUpdateRequest(const FramebufferUpdateRequest& request);
bool DecodeKeyEvent(const rfbKeyEventMsg& message, KeyEvent& out);
rfbKeyEventMsg EncodeKeyEvent(const KeyEvent& event);
bool DecodePointerEvent(const rfbPointerEventMsg& message, PointerEvent& out);
rfbPointerEventMsg EncodePointerEvent(const PointerEvent& event);
bool DecodeSetPixelFormat(const rfbSetPixelFormatMsg& message, rfbPixelFormat& out);
rfbSetPixelFormatMsg EncodeSetPixelFormat(const rfbPixelFormat& format);
bool DecodeSetEncodingsHeader(const rfbSetEncodingsMsg& message, unsigned int& count);
std::vector<CARD8> EncodeSetEncodings(const std::vector<CARD32>& encodings);
std::vector<CARD32> DecodeSetEncodingsPayload(const std::vector<CARD8>& payload);
std::vector<CARD8> EncodeClientCutText(const std::string& text);
bool DecodeClientCutTextHeader(const rfbClientCutTextMsg& message, unsigned int& length);
std::vector<CARD8> EncodeServerCutText(const std::string& text);
std::vector<CARD8> EncodeBell();
bool DecodeFileTransferHeader(const rfbFileTransferMsg& message, FileTransferMessage& out);
std::vector<CARD8> EncodeFileTransferAbort(CARD16 contentParam = 0, CARD32 size = 0);
std::vector<CARD8> EncodeFileTransferAccess(bool allowed);
std::vector<CARD8> EncodeFileTransferPacket(CARD8 contentType, CARD16 contentParam, CARD32 size, const std::vector<CARD8>& payload);
std::vector<CARD8> EncodeFileTransferPacket(CARD8 contentType, CARD16 contentParam, CARD32 size, const std::string& payload);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
