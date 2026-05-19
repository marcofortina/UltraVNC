// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_H
#define UVNC_WINVNC_PORTABLE_RFB_H

#include "rfb.h"
#include "vncPortableServerConfig.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

std::string ProtocolVersion38();
bool IsProtocolVersionMessage(const std::string& value);
const CARD32 kVeNCryptVersion = 2;
const CARD32 kVeNCryptSubTypeX509Vnc = 261;
std::vector<CARD8> SecurityTypesForConfig(const ServerConfig& config);
std::vector<CARD8> SecurityTypesForAuthMode(ServerAuthMode mode);
std::vector<CARD8> NoAuthSecurityTypes();
CARD32 AuthOkValue();
CARD32 AuthFailedValue();
rfbPixelFormat NetworkPixelFormat(const rfbPixelFormat& format);
std::vector<CARD8> ServerInitBytes(unsigned int width,
                                   unsigned int height,
                                   const rfbPixelFormat& format,
                                   const std::string& name);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_H
