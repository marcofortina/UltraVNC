// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDsmProvider.h"
#include "vncPortableServerConfig.h"

#include <cassert>
#include <string>
#include <vector>

using namespace uvnc::winvnc::portable;

#ifndef UVNC_TEST_DSM_PROVIDER_PATH
#error UVNC_TEST_DSM_PROVIDER_PATH must be defined
#endif

int main()
{
    std::string error;
    assert(!ValidateDsmProviderPath("relative-provider.so", &error));
    assert(error.find("absolute") != std::string::npos);

    DsmProvider provider;
    assert(provider.Load(UVNC_TEST_DSM_PROVIDER_PATH, &error));
    DsmProviderInfo info = provider.Info();
    assert(info.name == "test-dsm-provider");
    assert(info.capabilities.find("stream-transform") != std::string::npos);

    std::vector<CARD8> input;
    input.push_back(0x00);
    input.push_back(0xff);
    input.push_back(0x5a);
    std::vector<CARD8> output;
    assert(provider.Transform(DsmProviderDirection::ClientToServer, input, output, &error));
    assert(output.size() == input.size());
    assert(output[0] == 0xa5);
    assert(output[1] == 0x5a);
    assert(output[2] == 0xff);

    ServerConfig config;
    config.SetDsmProviderPath(UVNC_TEST_DSM_PROVIDER_PATH);
    assert(config.Validate(&error));
    config.SetDsmProviderPath("relative-provider.so");
    assert(!config.Validate(&error));
    return 0;
}
