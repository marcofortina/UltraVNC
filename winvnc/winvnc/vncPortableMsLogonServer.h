// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_MSLOGON_SERVER_H
#define UVNC_WINVNC_PORTABLE_MSLOGON_SERVER_H

#include "rfb.h"
#include "vncPortableExternalAuth.h"
#include "vncPortableRfbTransport.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

bool RunMsLogonIIServerAuthentication(RfbTransport& transport,
                                      const std::string& helperPath,
                                      std::string *error = nullptr);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_MSLOGON_SERVER_H
