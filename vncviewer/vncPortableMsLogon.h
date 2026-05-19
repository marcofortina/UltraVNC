// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_MSLOGON_H
#define UVNC_VNCVIEWER_PORTABLE_MSLOGON_H

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

const unsigned int kMsLogonIIExchangeBytes = 24;
const unsigned int kMsLogonIIPublicKeyBytes = 8;
const unsigned int kMsLogonIIUserBytes = 256;
const unsigned int kMsLogonIIPasswordBytes = 64;
const unsigned int kMsLogonIIResponseBytes = kMsLogonIIPublicKeyBytes + kMsLogonIIUserBytes + kMsLogonIIPasswordBytes;

struct MsLogonIIExchange {
    MsLogonIIExchange();

    unsigned long long generator;
    unsigned long long modulus;
    unsigned long long serverPublic;
};

bool DecodeMsLogonIIExchange(const std::vector<CARD8>& bytes, MsLogonIIExchange& exchange, std::string *error = nullptr);
bool EncodeMsLogonIIResponse(const MsLogonIIExchange& exchange,
                             const std::string& username,
                             const std::string& password,
                             std::vector<CARD8>& response,
                             std::string *error = nullptr);

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_MSLOGON_H
