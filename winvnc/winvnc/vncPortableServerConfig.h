// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H
#define UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H

#include "rfb.h"
#include "vncPortableFramebufferPattern.h"
#include "vncPortableFileTransfer.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

enum class ServerAuthMode {
    NoAuth,
    VncPassword
};

enum class TransportSecurityMode {
    None,
    VeNCryptX509Vnc
};

const char *ServerAuthModeName(ServerAuthMode mode);
bool ParseServerAuthMode(const std::string& value, ServerAuthMode& mode);
const char *TransportSecurityModeName(TransportSecurityMode mode);
bool ParseTransportSecurityMode(const std::string& value, TransportSecurityMode& mode);

class ServerConfig {
public:
    ServerConfig();

    const std::string& BindAddress() const { return bindAddress_; }
    unsigned short Port() const { return port_; }
    unsigned int Width() const { return width_; }
    unsigned int Height() const { return height_; }
    const std::string& DesktopName() const { return desktopName_; }
    unsigned char FillByte() const { return fillByte_; }
    FramebufferPattern Pattern() const { return pattern_; }
    rfbPixelFormat PixelFormat() const { return format_; }
    ServerAuthMode AuthMode() const { return authMode_; }
    const std::string& VncPassword() const { return vncPassword_; }
    bool AllowNoAuth() const { return allowNoAuth_; }
    bool AllowPublicNoAuth() const { return allowPublicNoAuth_; }
    bool AllowUnencryptedPublic() const { return allowUnencryptedPublic_; }
    bool BellOnConnect() const { return bellOnConnect_; }
    const std::string& ServerCutText() const { return serverCutText_; }
    bool ExtendedClipboardEnabled() const { return extendedClipboardEnabled_; }
    unsigned int ExtendedClipboardTextLimit() const { return extendedClipboardTextLimit_; }
    unsigned int MaxSharedClients() const { return maxSharedClients_; }
    bool EnableFileTransfer() const { return fileTransferMode_ != FileTransferMode::Disabled; }
    FileTransferMode FileTransferModeValue() const { return fileTransferMode_; }
    unsigned int FileTransferPayloadLimit() const { return fileTransferPayloadLimit_; }
    const std::string& FileTransferRoot() const { return fileTransferRoot_; }
    bool FileTransferAllowOverwrite() const { return fileTransferAllowOverwrite_; }
    unsigned int FileTransferRecursiveMaxDepth() const { return fileTransferRecursiveMaxDepth_; }
    unsigned int FileTransferRecursiveMaxEntries() const { return fileTransferRecursiveMaxEntries_; }
    TransportSecurityMode TransportSecurity() const { return transportSecurity_; }
    const std::string& TlsCertificateFile() const { return tlsCertificateFile_; }
    const std::string& TlsPrivateKeyFile() const { return tlsPrivateKeyFile_; }
    unsigned int UpdatePacingMs() const { return updatePacingMs_; }

    void SetBindAddress(const std::string& bindAddress) { bindAddress_ = bindAddress; }
    void SetPort(unsigned short port) { port_ = port; }
    void SetSize(unsigned int width, unsigned int height);
    void SetDesktopName(const std::string& desktopName) { desktopName_ = desktopName; }
    void SetFillByte(unsigned char fillByte) { fillByte_ = fillByte; }
    void SetPattern(FramebufferPattern pattern) { pattern_ = pattern; }
    void SetPixelFormat(const rfbPixelFormat& format) { format_ = format; }
    void SetAuthMode(ServerAuthMode mode) { authMode_ = mode; }
    void SetVncPassword(const std::string& password) { vncPassword_ = password; }
    void SetAllowNoAuth(bool allow) { allowNoAuth_ = allow; }
    void SetAllowPublicNoAuth(bool allow) { allowPublicNoAuth_ = allow; }
    void SetAllowUnencryptedPublic(bool allow) { allowUnencryptedPublic_ = allow; }
    void SetBellOnConnect(bool enable) { bellOnConnect_ = enable; }
    void SetServerCutText(const std::string& text) { serverCutText_ = text; }
    void SetExtendedClipboardEnabled(bool enable) { extendedClipboardEnabled_ = enable; }
    void SetExtendedClipboardTextLimit(unsigned int bytes) { extendedClipboardTextLimit_ = bytes; }
    void SetMaxSharedClients(unsigned int maxClients) { maxSharedClients_ = maxClients; }
    void SetEnableFileTransfer(bool enable) { fileTransferMode_ = enable ? FileTransferMode::RejectOnly : FileTransferMode::Disabled; }
    void SetFileTransferMode(FileTransferMode mode) { fileTransferMode_ = mode; }
    void SetFileTransferPayloadLimit(unsigned int bytes) { fileTransferPayloadLimit_ = bytes; }
    void SetFileTransferRoot(const std::string& root) { fileTransferRoot_ = root; }
    void SetFileTransferAllowOverwrite(bool allow) { fileTransferAllowOverwrite_ = allow; }
    void SetFileTransferRecursiveMaxDepth(unsigned int depth) { fileTransferRecursiveMaxDepth_ = depth; }
    void SetFileTransferRecursiveMaxEntries(unsigned int entries) { fileTransferRecursiveMaxEntries_ = entries; }
    void SetTransportSecurity(TransportSecurityMode mode) { transportSecurity_ = mode; }
    void SetTlsCertificateFile(const std::string& path) { tlsCertificateFile_ = path; }
    void SetTlsPrivateKeyFile(const std::string& path) { tlsPrivateKeyFile_ = path; }
    void SetUpdatePacingMs(unsigned int milliseconds) { updatePacingMs_ = milliseconds; }

    bool Validate(std::string *error = nullptr) const;

    static rfbPixelFormat DefaultPixelFormat();

private:
    std::string bindAddress_;
    unsigned short port_;
    unsigned int width_;
    unsigned int height_;
    std::string desktopName_;
    unsigned char fillByte_;
    FramebufferPattern pattern_;
    rfbPixelFormat format_;
    ServerAuthMode authMode_;
    unsigned int maxSharedClients_;
    std::string vncPassword_;
    bool allowNoAuth_;
    bool allowPublicNoAuth_;
    bool allowUnencryptedPublic_;
    bool bellOnConnect_;
    std::string serverCutText_;
    bool extendedClipboardEnabled_;
    unsigned int extendedClipboardTextLimit_;
    FileTransferMode fileTransferMode_;
    unsigned int fileTransferPayloadLimit_;
    std::string fileTransferRoot_;
    bool fileTransferAllowOverwrite_;
    unsigned int fileTransferRecursiveMaxDepth_;
    unsigned int fileTransferRecursiveMaxEntries_;
    TransportSecurityMode transportSecurity_;
    std::string tlsCertificateFile_;
    std::string tlsPrivateKeyFile_;
    unsigned int updatePacingMs_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H
