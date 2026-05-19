// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMsLogonServer.h"

#include <algorithm>
#include <cstring>
#include <vector>

extern "C" {
#include "d3des.h"
}

namespace uvnc {
namespace winvnc {
namespace portable {
namespace {

const unsigned int kExchangeBytes = 24;
const unsigned int kPublicKeyBytes = 8;
const unsigned int kUserBytes = 256;
const unsigned int kPasswordBytes = 64;
const unsigned int kResponseBytes = kPublicKeyBytes + kUserBytes + kPasswordBytes;

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

void DecryptMsLogonBlock(CARD8 *where, int length, CARD8 *key)
{
    CARD8 previous[kPublicKeyBytes] = {};
    deskey(key, DE1);
    for (int i = 0; i < length; i += 8) {
        CARD8 encrypted[kPublicKeyBytes] = {};
        std::memcpy(encrypted, where + i, kPublicKeyBytes);
        des(where + i, where + i);
        for (int j = 0; j < 8; ++j) {
            where[i + j] ^= (i == 0) ? key[j] : previous[j];
        }
        std::memcpy(previous, encrypted, kPublicKeyBytes);
    }
}

std::string NullTerminatedString(const CARD8 *data, std::size_t size)
{
    const CARD8 *end = std::find(data, data + size, 0);
    return std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(end));
}

} // namespace

bool RunMsLogonIIServerAuthentication(RfbTransport& transport,
                                      const std::string& helperPath,
                                      std::string *error)
{
    // Small deterministic DH group used by the historical UltraVNC MSLogonII wire
    // format. The external helper remains the trust boundary; DH only protects the
    // password on the RFB authentication exchange.
    const unsigned long long generator = 5;
    const unsigned long long modulus = 2147483647ULL;
    const unsigned long long serverPrivate = 918273645ULL;
    const unsigned long long serverPublic = PowMod(generator, serverPrivate, modulus);

    std::vector<CARD8> exchange(kExchangeBytes, 0);
    WriteU64BE(generator, exchange.data());
    WriteU64BE(modulus, exchange.data() + 8);
    WriteU64BE(serverPublic, exchange.data() + 16);
    if (!transport.WriteAll(exchange.data(), exchange.size())) {
        SetError(error, "failed to send MSLogonII exchange");
        return false;
    }

    std::vector<CARD8> response(kResponseBytes, 0);
    if (!transport.ReadExact(response.data(), response.size())) {
        SetError(error, "failed to read MSLogonII response");
        return false;
    }

    const unsigned long long clientPublic = ReadU64BE(response.data());
    const unsigned long long sharedKey = PowMod(clientPublic, serverPrivate, modulus);
    CARD8 key[kPublicKeyBytes] = {};
    WriteU64BE(sharedKey, key);

    CARD8 user[kUserBytes] = {};
    CARD8 password[kPasswordBytes] = {};
    std::copy(response.begin() + kPublicKeyBytes,
              response.begin() + kPublicKeyBytes + kUserBytes,
              user);
    std::copy(response.begin() + kPublicKeyBytes + kUserBytes,
              response.end(),
              password);
    DecryptMsLogonBlock(user, kUserBytes, key);
    DecryptMsLogonBlock(password, kPasswordBytes, key);

    ExternalAuthRequest request;
    request.username = NullTerminatedString(user, sizeof(user));
    request.password = NullTerminatedString(password, sizeof(password));
    request.method = "mslogon-ii";
    if (!RunExternalAuthHelper(helperPath, request, error)) {
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
