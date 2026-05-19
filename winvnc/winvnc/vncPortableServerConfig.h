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

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

enum class ServerAuthMode {
    NoAuth,
    VncPassword
};

const char *ServerAuthModeName(ServerAuthMode mode);
bool ParseServerAuthMode(const std::string& value, ServerAuthMode& mode);

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
    std::string vncPassword_;
    bool allowNoAuth_;
    bool allowPublicNoAuth_;
    bool allowUnencryptedPublic_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_SERVER_CONFIG_H
