// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDsmProvider.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

using namespace uvnc::winvnc::portable;

#ifndef UVNC_SECUREVNC_PROVIDER_PATH
#error UVNC_SECUREVNC_PROVIDER_PATH must be defined
#endif

int main()
{
    DsmProvider provider;
    std::string error;
    assert(provider.Load(UVNC_SECUREVNC_PROVIDER_PATH, &error));
    DsmProviderInfo info = provider.Info();
    assert(info.name == "native-securevnc-aes256ctr");
    assert(info.capabilities.find("aes-256-ctr") != std::string::npos);

    std::vector<CARD8> input;
    input.push_back('U');
    input.push_back('V');
    input.push_back('N');
    input.push_back('C');

    std::vector<CARD8> encrypted;
    unsetenv("UVNC_SECUREVNC_PROVIDER_KEY_HEX");
    assert(!provider.Transform(DsmProviderDirection::ClientToServer, input, encrypted, &error));

    setenv("UVNC_SECUREVNC_PROVIDER_KEY_HEX", "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff", 1);
    assert(provider.Transform(DsmProviderDirection::ClientToServer, input, encrypted, &error));
    assert(encrypted.size() == input.size());
    assert(encrypted != input);

    std::vector<CARD8> decrypted;
    assert(provider.Transform(DsmProviderDirection::ClientToServer, encrypted, decrypted, &error));
    assert(decrypted == input);
    unsetenv("UVNC_SECUREVNC_PROVIDER_KEY_HEX");
    return 0;
}
