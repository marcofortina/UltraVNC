// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMsLogon.h"

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include "vncauth.h"
}

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) *error = message;
}

unsigned long long ReadU64BE(const CARD8 *bytes)
{
    unsigned long long value = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<unsigned long long>(bytes[i]);
    }
    return value;
}

void WriteU64BE(unsigned long long value, CARD8 *bytes)
{
    for (int i = 7; i >= 0; --i) {
        bytes[i] = static_cast<CARD8>(value & 0xffULL);
        value >>= 8;
    }
}

unsigned long long PowMod(unsigned long long base, unsigned long long exponent, unsigned long long modulus)
{
    if (modulus == 0) return 0;
    unsigned long long result = 1 % modulus;
    base %= modulus;
    while (exponent > 0) {
        if (exponent & 1ULL) {
            result = static_cast<unsigned long long>((static_cast<unsigned __int128>(result) * base) % modulus);
        }
        base = static_cast<unsigned long long>((static_cast<unsigned __int128>(base) * base) % modulus);
        exponent >>= 1;
    }
    return result;
}

unsigned long long GeneratePrivateExponent()
{
    std::random_device rd;
    unsigned long long value = (static_cast<unsigned long long>(rd()) << 32) ^ rd();
    value &= 0x7fffffffULL;
    return value < 2 ? 7 : value;
}

} // namespace

MsLogonIIExchange::MsLogonIIExchange()
    : generator(0), modulus(0), serverPublic(0)
{
}

bool DecodeMsLogonIIExchange(const std::vector<CARD8>& bytes, MsLogonIIExchange& exchange, std::string *error)
{
    if (bytes.size() != kMsLogonIIExchangeBytes) {
        SetError(error, "MSLogonII exchange must be exactly 24 bytes");
        return false;
    }
    exchange.generator = ReadU64BE(bytes.data());
    exchange.modulus = ReadU64BE(bytes.data() + 8);
    exchange.serverPublic = ReadU64BE(bytes.data() + 16);
    if (exchange.generator < 2 || exchange.modulus < 3 || exchange.serverPublic == 0) {
        SetError(error, "MSLogonII exchange contains invalid Diffie-Hellman values");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool EncodeMsLogonIIResponse(const MsLogonIIExchange& exchange,
                             const std::string& username,
                             const std::string& password,
                             std::vector<CARD8>& response,
                             std::string *error)
{
    response.clear();
    if (username.empty()) {
        SetError(error, "MSLogonII requires a username");
        return false;
    }
    if (username.size() >= kMsLogonIIUserBytes) {
        SetError(error, "MSLogonII username is too long");
        return false;
    }
    if (password.empty()) {
        SetError(error, "MSLogonII requires a password");
        return false;
    }
    if (password.size() >= kMsLogonIIPasswordBytes) {
        SetError(error, "MSLogonII password is too long");
        return false;
    }
    if (exchange.generator < 2 || exchange.modulus < 3 || exchange.serverPublic == 0) {
        SetError(error, "MSLogonII exchange contains invalid Diffie-Hellman values");
        return false;
    }

    const unsigned long long clientPrivate = GeneratePrivateExponent();
    const unsigned long long clientPublic = PowMod(exchange.generator, clientPrivate, exchange.modulus);
    const unsigned long long sharedKey = PowMod(exchange.serverPublic, clientPrivate, exchange.modulus);

    CARD8 key[kMsLogonIIPublicKeyBytes] = {};
    WriteU64BE(sharedKey, key);

    CARD8 encryptedUser[kMsLogonIIUserBytes] = {};
    CARD8 encryptedPassword[kMsLogonIIPasswordBytes] = {};
    std::copy(username.begin(), username.end(), encryptedUser);
    std::copy(password.begin(), password.end(), encryptedPassword);

    vncEncryptBytes2(encryptedUser, kMsLogonIIUserBytes, key);
    vncEncryptBytes2(encryptedPassword, kMsLogonIIPasswordBytes, key);

    response.assign(kMsLogonIIResponseBytes, 0);
    WriteU64BE(clientPublic, response.data());
    std::copy(encryptedUser, encryptedUser + kMsLogonIIUserBytes, response.begin() + kMsLogonIIPublicKeyBytes);
    std::copy(encryptedPassword, encryptedPassword + kMsLogonIIPasswordBytes, response.begin() + kMsLogonIIPublicKeyBytes + kMsLogonIIUserBytes);
    if (error) error->clear();
    return true;
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
