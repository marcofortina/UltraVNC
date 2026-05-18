// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableVncAuth.h"

extern "C" {
#include "rfb/vncauth.h"
}

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

} // namespace

bool EncryptVncAuthChallenge(const std::vector<unsigned char>& challenge,
                             const std::string& password,
                             std::vector<unsigned char>& response,
                             std::string *error)
{
    if (challenge.size() != CHALLENGESIZE) {
        SetError(error, "invalid VNCAuth challenge length");
        return false;
    }
    if (password.empty()) {
        SetError(error, "VNCAuth password must not be empty");
        return false;
    }

    response = challenge;
    char passwordBuffer[MAXPWLEN + 1] = {};
    const std::size_t copyLength = std::min<std::size_t>(password.size(), MAXPWLEN);
    std::memcpy(passwordBuffer, password.data(), copyLength);
    vncEncryptBytes(response.data(), passwordBuffer);
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
