// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerConfig.h"

#include "rfb.h"

#include <sstream>

#include <sys/stat.h>

namespace uvnc {
namespace vncviewer {
namespace portable {

const char *ViewerTransportSecurityModeName(ViewerTransportSecurityMode mode)
{
    switch (mode) {
    case ViewerTransportSecurityMode::None:
        return "none";
    case ViewerTransportSecurityMode::VeNCryptX509Vnc:
        return "vencrypt-x509-vnc";
    }
    return "unknown";
}

bool ParseViewerTransportSecurityMode(const std::string& value, ViewerTransportSecurityMode& mode)
{
    if (value == "none") {
        mode = ViewerTransportSecurityMode::None;
        return true;
    }
    if (value == "vencrypt-x509-vnc" || value == "vencrypt" || value == "tls-vnc") {
        mode = ViewerTransportSecurityMode::VeNCryptX509Vnc;
        return true;
    }
    return false;
}

const char *ViewerSecurityExtensionModeName(ViewerSecurityExtensionMode mode)
{
    switch (mode) {
    case ViewerSecurityExtensionMode::None:
        return "none";
    case ViewerSecurityExtensionMode::DsmPlugin:
        return "dsm-plugin";
    case ViewerSecurityExtensionMode::MsLogon:
        return "mslogon";
    case ViewerSecurityExtensionMode::SecureVncPlugin:
        return "securevnc-plugin";
    }
    return "unknown";
}

bool ParseViewerSecurityExtensionMode(const std::string& value, ViewerSecurityExtensionMode& mode)
{
    if (value == "none") {
        mode = ViewerSecurityExtensionMode::None;
        return true;
    }
    if (value == "dsm" || value == "dsm-plugin") {
        mode = ViewerSecurityExtensionMode::DsmPlugin;
        return true;
    }
    if (value == "mslogon" || value == "mslogon-i" || value == "mslogon-ii") {
        mode = ViewerSecurityExtensionMode::MsLogon;
        return true;
    }
    if (value == "securevnc" || value == "securevnc-plugin") {
        mode = ViewerSecurityExtensionMode::SecureVncPlugin;
        return true;
    }
    return false;
}

namespace {

bool FileExists(const std::string& path)
{
    struct stat st;
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

}

ViewerConfig::ViewerConfig()
    : host_("127.0.0.1"),
      port_(5900),
      shared_(true),
      requestUpdate_(false),
      viewOnly_(false),
      allowNoAuth_(true),
      password_(),
      continuousUpdates_(false),
      updateIntervalMs_(1000),
      encodings_(),
      socketTimeoutMs_(15000),
      transportSecurity_(ViewerTransportSecurityMode::None),
      tlsCaFile_(),
      tlsServerName_(),
      tlsVerifyPeer_(true),
      securityExtension_(ViewerSecurityExtensionMode::None),
      securityExtensionName_()
{
    encodings_.push_back(rfbEncodingRaw);
    encodings_.push_back(rfbEncodingCopyRect);
    encodings_.push_back(rfbEncodingHextile);
    encodings_.push_back(rfbEncodingZlib);
    encodings_.push_back(rfbEncodingZRLE);
    encodings_.push_back(rfbEncodingTight);
    encodings_.push_back(rfbEncodingRRE);
    encodings_.push_back(rfbEncodingCoRRE);
    encodings_.push_back(rfbEncodingNewFBSize);
    encodings_.push_back(rfbEncodingRichCursor);
    encodings_.push_back(rfbEncodingXCursor);
    encodings_.push_back(rfbEncodingPointerPos);
    encodings_.push_back(rfbEncodingLastRect);
    encodings_.push_back(rfbEncodingExtendedClipboard);
}

bool ViewerConfig::Validate(std::string *error) const
{
    if (host_.empty()) {
        if (error) *error = "viewer host must not be empty";
        return false;
    }
    if (port_ == 0) {
        if (error) *error = "viewer port must not be zero";
        return false;
    }
    if (updateIntervalMs_ == 0) {
        if (error) *error = "viewer update interval must not be zero";
        return false;
    }
    if (socketTimeoutMs_ == 0) {
        if (error) *error = "viewer socket timeout must not be zero";
        return false;
    }
    if (encodings_.empty()) {
        if (error) *error = "viewer encoding list must not be empty";
        return false;
    }
    if (securityExtension_ != ViewerSecurityExtensionMode::None) {
        if (error) *error = std::string("portable Linux viewer does not implement ") +
                           ViewerSecurityExtensionModeName(securityExtension_) +
                           "; use Windows viewer or wait for a native provider ABI";
        return false;
    }
    if (transportSecurity_ == ViewerTransportSecurityMode::VeNCryptX509Vnc) {
        if (password_.empty()) {
            if (error) *error = "viewer VeNCrypt/X509Vnc requires a VNCAuth password";
            return false;
        }
        if (tlsVerifyPeer_ && !tlsServerName_.empty() && tlsServerName_.find(' ') != std::string::npos) {
            if (error) *error = "viewer TLS server name must not contain spaces";
            return false;
        }
        if (tlsVerifyPeer_ && !FileExists(tlsCaFile_)) {
            if (error) *error = "viewer TLS peer verification requires a readable CA file";
            return false;
        }
    }
    if (error) error->clear();
    return true;
}

std::string ViewerConfigSummary(const ViewerConfig& config)
{
    std::ostringstream out;
    out << "host=" << config.Host() << "\n"
        << "port=" << config.Port() << "\n"
        << "shared=" << (config.Shared() ? "yes" : "no") << "\n"
        << "view_only=" << (config.ViewOnly() ? "yes" : "no") << "\n"
        << "allow_no_auth=" << (config.AllowNoAuth() ? "yes" : "no") << "\n"
        << "password_configured=" << (!config.Password().empty() ? "yes" : "no") << "\n"
        << "transport_security=" << ViewerTransportSecurityModeName(config.TransportSecurity()) << "\n"
        << "tls_verify_peer=" << (config.TlsVerifyPeer() ? "yes" : "no") << "\n"
        << "tls_server_name=" << config.TlsServerName() << "\n"
        << "security_extension=" << ViewerSecurityExtensionModeName(config.SecurityExtension()) << "\n"
        << "security_extension_name=" << config.SecurityExtensionName() << "\n"
        << "continuous_updates=" << (config.ContinuousUpdates() ? "yes" : "no") << "\n"
        << "update_interval_ms=" << config.UpdateIntervalMs() << "\n"
        << "socket_timeout_ms=" << config.SocketTimeoutMs() << "\n"
        << "encodings=" << config.Encodings().size() << "\n";
    return out.str();
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
