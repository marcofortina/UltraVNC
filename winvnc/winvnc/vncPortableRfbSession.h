// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_SESSION_H
#define UVNC_WINVNC_PORTABLE_RFB_SESSION_H

#include "vncPortableServerConfig.h"
#include "vncPortableTcp.h"

namespace uvnc {
namespace winvnc {
namespace portable {

class RfbServerSession {
public:
    bool RunHandshake(TcpSocket& socket, const ServerConfig& config) const;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_SESSION_H
