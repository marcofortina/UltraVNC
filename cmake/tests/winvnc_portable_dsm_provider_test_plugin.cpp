// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include <cstddef>

extern "C" unsigned int uvnc_dsm_provider_abi_version()
{
    return 1;
}

extern "C" const char *uvnc_dsm_provider_name()
{
    return "test-dsm-provider";
}

extern "C" const char *uvnc_dsm_provider_capabilities()
{
    return "stream-transform;securevnc-compatible-boundary";
}

extern "C" int uvnc_dsm_provider_transform(int, const unsigned char *input, std::size_t inputLength, unsigned char *output, std::size_t *outputLength)
{
    if (!outputLength || *outputLength < inputLength) {
        if (outputLength) *outputLength = inputLength;
        return 1;
    }
    for (std::size_t i = 0; i < inputLength; ++i) {
        output[i] = static_cast<unsigned char>(input[i] ^ 0xa5U);
    }
    *outputLength = inputLength;
    return 0;
}
