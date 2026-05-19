// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDsmProvider.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

bool HexNibble(char c, unsigned char& value)
{
    if (c >= '0' && c <= '9') {
        value = static_cast<unsigned char>(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        value = static_cast<unsigned char>(c - 'a' + 10);
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        value = static_cast<unsigned char>(c - 'A' + 10);
        return true;
    }
    return false;
}

bool DecodeHexKey(const char *hex, unsigned char *key, std::size_t keyLength)
{
    if (!hex) return false;
    const std::string input(hex);
    if (input.size() != keyLength * 2) return false;
    for (std::size_t i = 0; i < keyLength; ++i) {
        unsigned char high = 0;
        unsigned char low = 0;
        if (!HexNibble(input[i * 2], high) || !HexNibble(input[i * 2 + 1], low)) {
            return false;
        }
        key[i] = static_cast<unsigned char>((high << 4) | low);
    }
    return true;
}

void DirectionIv(int direction, unsigned char *iv, std::size_t ivLength)
{
    std::memset(iv, 0, ivLength);
    const char *label = direction == 0 ? "uvnc-securevnc-client-to-server" : "uvnc-securevnc-server-to-client";
    unsigned char digest[SHA256_DIGEST_LENGTH] = {};
    SHA256(reinterpret_cast<const unsigned char *>(label), std::strlen(label), digest);
    std::memcpy(iv, digest, ivLength < SHA256_DIGEST_LENGTH ? ivLength : SHA256_DIGEST_LENGTH);
}

} // namespace

extern "C" unsigned int uvnc_dsm_provider_abi_version()
{
    return uvnc::winvnc::portable::kDsmProviderAbiVersion;
}

extern "C" const char *uvnc_dsm_provider_name()
{
    return "native-securevnc-aes256ctr";
}

extern "C" const char *uvnc_dsm_provider_capabilities()
{
    return "securevnc-native;stream-transform;aes-256-ctr;key-env:UVNC_SECUREVNC_PROVIDER_KEY_HEX";
}

extern "C" int uvnc_dsm_provider_transform(int direction,
                                             const unsigned char *input,
                                             std::size_t inputLength,
                                             unsigned char *output,
                                             std::size_t *outputLength)
{
    if (!outputLength) return 1;
    if (*outputLength < inputLength) {
        *outputLength = inputLength;
        return 2;
    }
    if (inputLength > 0 && (!input || !output)) return 3;

    unsigned char key[32] = {};
    if (!DecodeHexKey(std::getenv("UVNC_SECUREVNC_PROVIDER_KEY_HEX"), key, sizeof(key))) {
        return 4;
    }
    unsigned char iv[16] = {};
    DirectionIv(direction, iv, sizeof(iv));

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return 5;
    int ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr, key, iv);
    int outLength = 0;
    int finalLength = 0;
    if (ok == 1 && inputLength > 0) {
        ok = EVP_EncryptUpdate(ctx, output, &outLength, input, static_cast<int>(inputLength));
    }
    if (ok == 1) {
        ok = EVP_EncryptFinal_ex(ctx, output + outLength, &finalLength);
    }
    EVP_CIPHER_CTX_free(ctx);
    if (ok != 1) return 6;
    *outputLength = static_cast<std::size_t>(outLength + finalLength);
    return 0;
}
