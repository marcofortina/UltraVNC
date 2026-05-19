// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_DSM_PROVIDER_H
#define UVNC_WINVNC_PORTABLE_DSM_PROVIDER_H

#include "rfb.h"

#include <cstddef>
#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

const unsigned int kDsmProviderAbiVersion = 1;

enum class DsmProviderDirection {
    ClientToServer = 0,
    ServerToClient = 1,
};

struct DsmProviderInfo {
    std::string path;
    std::string name;
    std::string capabilities;
};

class DsmProvider {
public:
    DsmProvider();
    ~DsmProvider();

    DsmProvider(const DsmProvider&) = delete;
    DsmProvider& operator=(const DsmProvider&) = delete;

    bool Load(const std::string& path, std::string *error = nullptr);
    bool Loaded() const { return handle_ != nullptr; }
    DsmProviderInfo Info() const;
    bool Transform(DsmProviderDirection direction,
                   const std::vector<CARD8>& input,
                   std::vector<CARD8>& output,
                   std::string *error = nullptr) const;
    void Reset();

private:
    void *handle_;
    std::string path_;
    std::string name_;
    std::string capabilities_;
    int (*transform_)(int direction,
                      const unsigned char *input,
                      std::size_t inputLength,
                      unsigned char *output,
                      std::size_t *outputLength);
};

bool ValidateDsmProviderPath(const std::string& path, std::string *error = nullptr);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

extern "C" {
using uvnc_dsm_provider_abi_version_fn = unsigned int (*)();
using uvnc_dsm_provider_name_fn = const char *(*)();
using uvnc_dsm_provider_capabilities_fn = const char *(*)();
using uvnc_dsm_provider_transform_fn = int (*)(int direction,
                                                const unsigned char *input,
                                                std::size_t inputLength,
                                                unsigned char *output,
                                                std::size_t *outputLength);
}

#endif // UVNC_WINVNC_PORTABLE_DSM_PROVIDER_H
