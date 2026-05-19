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
      lastCursorShapeFingerprint_(0),
      extendedClipboardEnabled_(config.ExtendedClipboardEnabled()),
      extendedClipboardTextLimit_(config.ExtendedClipboardTextLimit()),
      extendedClipboardCapsSent_(false),
      extendedClipboardRemoteCaps_(0),
      extendedClipboardRemoteTextLimit_(0),
      extendedClipboardTextAvailable_(false),
      sharedClientRequested_(true),
      clientInitReceived_(false),
      lastFramebufferWidth_(config.Width()),
      lastFramebufferHeight_(config.Height()),
      qualityLevel_(-1),
      compressLevel_(-1),
      scaleFactor_(1),
      fileTransferMode_(config.FileTransferModeValue()),
      fileTransferPayloadLimit_(config.FileTransferPayloadLimit()),
      fileTransferRoot_(config.FileTransferRoot()),
      fileTransferAllowOverwrite_(config.FileTransferAllowOverwrite()),
      fileTransferRecursiveMaxDepth_(config.FileTransferRecursiveMaxDepth()),
      fileTransferRecursiveMaxEntries_(config.FileTransferRecursiveMaxEntries()),
      fileUploadActive_(false),
      fileUploadPath_(),
      fileUploadFinalPath_(),
      fileUploadTemporaryPath_(),
      fileUploadBytes_(0)
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

bool RfbClientState::SupportsExtendedClipboard() const
{
    return extendedClipboardEnabled_ && SupportsEncoding(rfbEncodingExtendedClipboard);
}

bool RfbClientState::SupportsNewFramebufferSizeUpdates() const
{
    return SupportsEncoding(rfbEncodingNewFBSize);
}

bool RfbClientState::SupportsLastRect() const
{
    return SupportsEncoding(rfbEncodingLastRect);
}

void RfbClientState::SetPixelFormat(const rfbPixelFormat& format)
{
    pixelFormat_ = format;
}

void RfbClientState::SetEncodings(const std::vector<CARD32>& encodings)
{
    encodings_ = encodings;
    qualityLevel_ = -1;
    compressLevel_ = -1;
    for (std::vector<CARD32>::const_iterator it = encodings_.begin(); it != encodings_.end(); ++it) {
        if (*it >= rfbEncodingQualityLevel0 && *it <= rfbEncodingQualityLevel9) {
            qualityLevel_ = static_cast<int>(*it - rfbEncodingQualityLevel0);
        } else if (*it >= rfbEncodingCompressLevel0 && *it <= rfbEncodingCompressLevel9) {
            compressLevel_ = static_cast<int>(*it - rfbEncodingCompressLevel0);
        }
    }
    cursorShapeSent_ = false;
    lastCursorShapeFingerprint_ = 0;
    extendedClipboardCapsSent_ = false;
}

void RfbClientState::SetScaleFactor(unsigned int scale)
{
    scaleFactor_ = scale == 0 ? 1 : scale;
}

bool RfbClientState::CursorShapeChanged(CARD32 fingerprint) const
{
    return !cursorShapeSent_ || lastCursorShapeFingerprint_ != fingerprint;
}

void RfbClientState::MarkCursorShapeSent()
{
    MarkCursorShapeSent(0);
}

void RfbClientState::MarkCursorShapeSent(CARD32 fingerprint)
{
    cursorShapeSent_ = true;
    lastCursorShapeFingerprint_ = fingerprint;
}

void RfbClientState::MarkExtendedClipboardCapsSent()
{
    extendedClipboardCapsSent_ = true;
}

void RfbClientState::RecordExtendedClipboardRemoteCaps(CARD32 caps, unsigned int textLimit)
{
    extendedClipboardRemoteCaps_ = caps;
    extendedClipboardRemoteTextLimit_ = textLimit;
}

void RfbClientState::RecordExtendedClipboardNotify(CARD32 flags)
{
    extendedClipboardTextAvailable_ = (flags & clipText) != 0;
}

void RfbClientState::RecordClientInit(bool shared)
{
    sharedClientRequested_ = shared;
    clientInitReceived_ = true;
}

bool RfbClientState::FramebufferSizeChanged(unsigned int width, unsigned int height) const
{
    return lastFramebufferWidth_ != width || lastFramebufferHeight_ != height;
}

void RfbClientState::RecordFramebufferSize(unsigned int width, unsigned int height)
{
    lastFramebufferWidth_ = width;
    lastFramebufferHeight_ = height;
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

void RfbClientState::BeginFileUpload(const std::string& path)
{
    BeginFileUpload(path, path);
}

void RfbClientState::BeginFileUpload(const std::string& finalPath, const std::string& temporaryPath)
{
    fileUploadActive_ = true;
    fileUploadPath_ = temporaryPath;
    fileUploadFinalPath_ = finalPath;
    fileUploadTemporaryPath_ = temporaryPath;
    fileUploadBytes_ = 0;
}

void RfbClientState::AddFileUploadBytes(CARD32 bytes)
{
    fileUploadBytes_ += bytes;
}

void RfbClientState::EndFileUpload()
{
    fileUploadActive_ = false;
    fileUploadPath_.clear();
    fileUploadFinalPath_.clear();
    fileUploadTemporaryPath_.clear();
    fileUploadBytes_ = 0;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
