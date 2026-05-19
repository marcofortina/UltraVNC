// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableServerConfig.h"

#include <arpa/inet.h>
#include <cstring>


namespace {

bool IsValidIpv4BindAddress(const std::string& address)
{
    if (address.empty()) {
        return false;
    }
    in_addr parsed;
    return inet_pton(AF_INET, address.c_str(), &parsed) == 1;
}

} // namespace

namespace uvnc {
namespace winvnc {
namespace portable {

const char *ServerAuthModeName(ServerAuthMode mode)
{
    switch (mode) {
    case ServerAuthMode::NoAuth:
        return "none";
    case ServerAuthMode::VncPassword:
        return "vnc-password";
    }
    return "unknown";
}

bool ParseServerAuthMode(const std::string& value, ServerAuthMode& mode)
{
    if (value == "none" || value == "no-auth") {
        mode = ServerAuthMode::NoAuth;
        return true;
    }
    if (value == "vnc-password" || value == "vncauth") {
        mode = ServerAuthMode::VncPassword;
        return true;
    }
    return false;
}

const char *TransportSecurityModeName(TransportSecurityMode mode)
{
    switch (mode) {
    case TransportSecurityMode::None:
        return "none";
    case TransportSecurityMode::VeNCryptX509Vnc:
        return "vencrypt-x509-vnc";
    }
    return "unknown";
}

bool ParseTransportSecurityMode(const std::string& value, TransportSecurityMode& mode)
{
    if (value == "none" || value == "disabled") {
        mode = TransportSecurityMode::None;
        return true;
    }
    if (value == "vencrypt-x509-vnc" || value == "vencrypt" || value == "tls-vnc") {
        mode = TransportSecurityMode::VeNCryptX509Vnc;
        return true;
    }
    return false;
}

ServerConfig::ServerConfig()
    : bindAddress_("127.0.0.1"),
      port_(0),
      width_(640),
      height_(480),
      desktopName_("UltraVNC native Linux memory server"),
      fillByte_(0x22),
      pattern_(FramebufferPattern::Solid),
      format_(DefaultPixelFormat()),
      authMode_(ServerAuthMode::NoAuth),
      maxSharedClients_(8),
      vncPassword_(),
      allowNoAuth_(false),
      allowPublicNoAuth_(false),
      allowUnencryptedPublic_(false),
      bellOnConnect_(false),
      serverCutText_(),
      extendedClipboardEnabled_(true),
      extendedClipboardTextLimit_(10U * 1024U * 1024U),
      fileTransferMode_(FileTransferMode::Disabled),
      fileTransferPayloadLimit_(DefaultFileTransferPayloadLimit()),
      fileTransferRoot_(),
      fileTransferAllowOverwrite_(false),
      fileTransferRecursiveMaxDepth_(32),
      fileTransferRecursiveMaxEntries_(16384),
      transportSecurity_(TransportSecurityMode::None),
      tlsCertificateFile_(),
      tlsPrivateKeyFile_(),
      updatePacingMs_(0)
{
}

void ServerConfig::SetSize(unsigned int width, unsigned int height)
{
    width_ = width;
    height_ = height;
}

bool ServerConfig::Validate(std::string *error) const
{
    if (!IsValidIpv4BindAddress(bindAddress_)) {
        if (error) *error = "bind address must be a valid IPv4 address";
        return false;
    }
    if (width_ == 0 || height_ == 0) {
        if (error) *error = "framebuffer size must be non-zero";
        return false;
    }
    if (width_ > 16384 || height_ > 16384) {
        if (error) *error = "framebuffer size is too large";
        return false;
    }
    if (desktopName_.empty()) {
        if (error) *error = "desktop name must not be empty";
        return false;
    }
    if (desktopName_.size() > 1024) {
        if (error) *error = "desktop name is too long";
        return false;
    }
    if (maxSharedClients_ == 0 || maxSharedClients_ > 64) {
        if (error) *error = "max shared clients must be between 1 and 64";
        return false;
    }
    if (serverCutText_.size() > extendedClipboardTextLimit_) {
        if (error) *error = "server cut text exceeds extended clipboard text limit";
        return false;
    }
    if (extendedClipboardTextLimit_ == 0 || extendedClipboardTextLimit_ > 100U * 1024U * 1024U) {
        if (error) *error = "extended clipboard text limit must be between 1 and 104857600 bytes";
        return false;
    }
    if (format_.bitsPerPixel != 8 && format_.bitsPerPixel != 16 && format_.bitsPerPixel != 32) {
        if (error) *error = "bitsPerPixel must be 8, 16, or 32";
        return false;
    }
    if (format_.bitsPerPixel % 8 != 0) {
        if (error) *error = "bitsPerPixel must be byte-aligned";
        return false;
    }
    if (authMode_ == ServerAuthMode::VncPassword) {
        if (vncPassword_.empty()) {
            if (error) *error = "VNCAuth password must not be empty";
            return false;
        }
        if (vncPassword_.size() > 8) {
            if (error) *error = "VNCAuth password must be at most 8 bytes";
            return false;
        }
    }
    if (fileTransferPayloadLimit_ == 0 || fileTransferPayloadLimit_ > 16U * 1024U * 1024U) {
        if (error) *error = "file-transfer payload guard limit must be between 1 and 16777216 bytes";
        return false;
    }
    if (transportSecurity_ == TransportSecurityMode::VeNCryptX509Vnc) {
        if (authMode_ != ServerAuthMode::VncPassword) {
            if (error) *error = "VeNCrypt TLS mode requires auth=vnc-password";
            return false;
        }
        if (tlsCertificateFile_.empty() || tlsPrivateKeyFile_.empty()) {
            if (error) *error = "VeNCrypt TLS mode requires tls_certificate_file and tls_private_key_file";
            return false;
        }
        if (tlsCertificateFile_[0] != '/' || tlsPrivateKeyFile_[0] != '/') {
            if (error) *error = "TLS certificate and private key paths must be absolute";
            return false;
        }
    }
    if (fileTransferRecursiveMaxDepth_ == 0 || fileTransferRecursiveMaxDepth_ > 256) {
        if (error) *error = "file-transfer recursive max depth must be between 1 and 256";
        return false;
    }
    if (updatePacingMs_ > 5000) {
        if (error) *error = "update pacing must be between 0 and 5000 milliseconds";
        return false;
    }
    if (fileTransferRecursiveMaxEntries_ == 0 || fileTransferRecursiveMaxEntries_ > 1048576) {
        if (error) *error = "file-transfer recursive max entries must be between 1 and 1048576";
        return false;
    }
    if ((fileTransferMode_ == FileTransferMode::ReadOnly || fileTransferMode_ == FileTransferMode::ReadWrite) && fileTransferRoot_.empty()) {
        if (error) *error = "file-transfer root is required for read-only/read-write modes";
        return false;
    }
    if (!fileTransferRoot_.empty() && fileTransferRoot_[0] != '/') {
        if (error) *error = "file-transfer root must be an absolute path";
        return false;
    }
    return true;
}

rfbPixelFormat ServerConfig::DefaultPixelFormat()
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.bigEndian = 0;
    format.trueColour = 1;
    format.redMax = 255;
    format.greenMax = 255;
    format.blueMax = 255;
    format.redShift = 16;
    format.greenShift = 8;
    format.blueShift = 0;
    return format;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
