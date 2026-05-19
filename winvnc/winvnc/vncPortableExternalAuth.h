// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_EXTERNAL_AUTH_H
#define UVNC_WINVNC_PORTABLE_EXTERNAL_AUTH_H

#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

struct ExternalAuthRequest {
    std::string username;
    std::string password;
    std::string method;
};

bool RunExternalAuthHelper(const std::string& helperPath,
                           const ExternalAuthRequest& request,
                           std::string *error = nullptr);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_EXTERNAL_AUTH_H
