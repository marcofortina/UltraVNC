// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VNC_AUTH_H
#define UVNC_VNCVIEWER_PORTABLE_VNC_AUTH_H

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

bool EncryptVncAuthChallenge(const std::vector<unsigned char>& challenge,
                             const std::string& password,
                             std::vector<unsigned char>& response,
                             std::string *error = nullptr);

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VNC_AUTH_H
