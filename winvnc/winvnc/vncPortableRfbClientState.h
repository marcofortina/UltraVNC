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
#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class RfbClientState {
public:
    explicit RfbClientState(const ServerConfig& config = ServerConfig());

    const rfbPixelFormat& PixelFormat() const { return pixelFormat_; }
    const std::vector<CARD32>& Encodings() const { return encodings_; }
    bool SupportsEncoding(CARD32 encoding) const;
    bool SupportsPointerPositionUpdates() const;
    bool SupportsRichCursorUpdates() const;
    bool SupportsXCursorUpdates() const;
    bool SupportsCursorShapeUpdates() const;
    bool SupportsExtendedClipboard() const;
bool SupportsNewFramebufferSizeUpdates() const;
    bool ExtendedClipboardCapsSent() const { return extendedClipboardCapsSent_; }
    CARD32 ExtendedClipboardRemoteCaps() const { return extendedClipboardRemoteCaps_; }
    unsigned int ExtendedClipboardRemoteTextLimit() const { return extendedClipboardRemoteTextLimit_; }
    bool ExtendedClipboardTextAvailable() const { return extendedClipboardTextAvailable_; }
    bool ExtendedClipboardEnabled() const { return extendedClipboardEnabled_; }
    unsigned int ExtendedClipboardTextLimit() const { return extendedClipboardTextLimit_; }
    bool CursorShapeSent() const { return cursorShapeSent_; }
    CARD32 LastCursorShapeFingerprint() const { return lastCursorShapeFingerprint_; }
    bool CursorShapeChanged(CARD32 fingerprint) const;
    bool SharedClientRequested() const { return sharedClientRequested_; }
    bool ClientInitReceived() const { return clientInitReceived_; }
unsigned int LastFramebufferWidth() const { return lastFramebufferWidth_; }
unsigned int LastFramebufferHeight() const { return lastFramebufferHeight_; }
bool FramebufferSizeChanged(unsigned int width, unsigned int height) const;
    const KeyEvent& LastKeyEvent() const { return lastKeyEvent_; }
    const PointerEvent& LastPointerEvent() const { return lastPointerEvent_; }
    unsigned int KeyEventCount() const { return keyEventCount_; }
    unsigned int PointerEventCount() const { return pointerEventCount_; }
    unsigned int ClientCutTextMessages() const { return clientCutTextMessages_; }
    unsigned int ClientCutTextBytes() const { return clientCutTextBytes_; }
    const std::string& LastClientCutText() const { return lastClientCutText_; }
    const std::string& LastServerCutText() const { return lastServerCutText_; }
    FileTransferMode FileTransferModeValue() const { return fileTransferMode_; }
    unsigned int FileTransferPayloadLimit() const { return fileTransferPayloadLimit_; }
    const std::string& FileTransferRoot() const { return fileTransferRoot_; }
    bool FileTransferAllowOverwrite() const { return fileTransferAllowOverwrite_; }
    unsigned int FileTransferRecursiveMaxDepth() const { return fileTransferRecursiveMaxDepth_; }
    unsigned int FileTransferRecursiveMaxEntries() const { return fileTransferRecursiveMaxEntries_; }
    bool FileUploadActive() const { return fileUploadActive_; }
    const std::string& FileUploadPath() const { return fileUploadPath_; }
    const std::string& FileUploadFinalPath() const { return fileUploadFinalPath_; }
    const std::string& FileUploadTemporaryPath() const { return fileUploadTemporaryPath_; }
    CARD32 FileUploadBytes() const { return fileUploadBytes_; }

    void SetPixelFormat(const rfbPixelFormat& format);
    void SetEncodings(const std::vector<CARD32>& encodings);
    void MarkCursorShapeSent();
    void MarkCursorShapeSent(CARD32 fingerprint);
    void MarkExtendedClipboardCapsSent();
    void RecordExtendedClipboardRemoteCaps(CARD32 caps, unsigned int textLimit = 0);
    void RecordExtendedClipboardNotify(CARD32 flags);
    void RecordClientInit(bool shared);
void RecordFramebufferSize(unsigned int width, unsigned int height);
    void RecordKeyEvent(const KeyEvent& event);
    void RecordPointerEvent(const PointerEvent& event);
    void RecordClientCutText(unsigned int bytes);
    void RecordClientCutText(const std::string& text);
    void RecordServerCutTextSent(const std::string& text);
    void BeginFileUpload(const std::string& path);
    void BeginFileUpload(const std::string& finalPath, const std::string& temporaryPath);
    void AddFileUploadBytes(CARD32 bytes);
    void EndFileUpload();

private:
    rfbPixelFormat pixelFormat_;
    std::vector<CARD32> encodings_;
    KeyEvent lastKeyEvent_;
    PointerEvent lastPointerEvent_;
    unsigned int keyEventCount_;
    unsigned int pointerEventCount_;
    unsigned int clientCutTextMessages_;
    unsigned int clientCutTextBytes_;
    std::string lastClientCutText_;
    std::string lastServerCutText_;
    bool cursorShapeSent_;
    CARD32 lastCursorShapeFingerprint_;
    bool extendedClipboardEnabled_;
    unsigned int extendedClipboardTextLimit_;
    bool extendedClipboardCapsSent_;
    CARD32 extendedClipboardRemoteCaps_;
    unsigned int extendedClipboardRemoteTextLimit_;
    bool extendedClipboardTextAvailable_;
    bool sharedClientRequested_;
    bool clientInitReceived_;
unsigned int lastFramebufferWidth_;
unsigned int lastFramebufferHeight_;
    FileTransferMode fileTransferMode_;
    unsigned int fileTransferPayloadLimit_;
    std::string fileTransferRoot_;
    bool fileTransferAllowOverwrite_;
    unsigned int fileTransferRecursiveMaxDepth_;
    unsigned int fileTransferRecursiveMaxEntries_;
    bool fileUploadActive_;
    std::string fileUploadPath_;
    std::string fileUploadFinalPath_;
    std::string fileUploadTemporaryPath_;
    CARD32 fileUploadBytes_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_CLIENT_STATE_H
