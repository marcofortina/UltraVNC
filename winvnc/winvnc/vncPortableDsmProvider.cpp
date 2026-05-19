// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableDsmProvider.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) *error = message;
}

std::string DlErrorText()
{
    const char *message = dlerror();
    return message ? std::string(message) : std::string("unknown dlopen error");
}

} // namespace

DsmProvider::DsmProvider()
    : handle_(nullptr), transform_(nullptr)
{
}

DsmProvider::~DsmProvider()
{
    Reset();
}

void DsmProvider::Reset()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    path_.clear();
    name_.clear();
    capabilities_.clear();
    transform_ = nullptr;
}

bool ValidateDsmProviderPath(const std::string& path, std::string *error)
{
    if (path.empty()) {
        SetError(error, "DSM provider path must not be empty");
        return false;
    }
    if (path[0] != '/') {
        SetError(error, "DSM provider path must be absolute");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool DsmProvider::Load(const std::string& path, std::string *error)
{
    Reset();
    if (!ValidateDsmProviderPath(path, error)) {
        return false;
    }

    dlerror();
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        SetError(error, std::string("failed to load DSM provider: ") + DlErrorText());
        return false;
    }

    uvnc_dsm_provider_abi_version_fn abi = reinterpret_cast<uvnc_dsm_provider_abi_version_fn>(dlsym(handle, "uvnc_dsm_provider_abi_version"));
    uvnc_dsm_provider_name_fn name = reinterpret_cast<uvnc_dsm_provider_name_fn>(dlsym(handle, "uvnc_dsm_provider_name"));
    uvnc_dsm_provider_capabilities_fn capabilities = reinterpret_cast<uvnc_dsm_provider_capabilities_fn>(dlsym(handle, "uvnc_dsm_provider_capabilities"));
    uvnc_dsm_provider_transform_fn transform = reinterpret_cast<uvnc_dsm_provider_transform_fn>(dlsym(handle, "uvnc_dsm_provider_transform"));
    if (!abi || !name || !transform) {
        dlclose(handle);
        SetError(error, "DSM provider is missing required ABI symbols");
        return false;
    }
    if (abi() != kDsmProviderAbiVersion) {
        dlclose(handle);
        SetError(error, "DSM provider ABI version mismatch");
        return false;
    }

    handle_ = handle;
    path_ = path;
    name_ = name() ? name() : "unknown";
    capabilities_ = capabilities && capabilities() ? capabilities() : "stream-transform";
    transform_ = transform;
    if (error) error->clear();
    return true;
}

DsmProviderInfo DsmProvider::Info() const
{
    DsmProviderInfo info;
    info.path = path_;
    info.name = name_;
    info.capabilities = capabilities_;
    return info;
}

bool DsmProvider::Transform(DsmProviderDirection direction,
                            const std::vector<CARD8>& input,
                            std::vector<CARD8>& output,
                            std::string *error) const
{
    if (!transform_) {
        SetError(error, "DSM provider is not loaded");
        return false;
    }
    std::size_t outputLength = input.size() + 1024;
    output.assign(outputLength, 0);
    const int rc = transform_(static_cast<int>(direction), input.empty() ? nullptr : input.data(), input.size(), output.data(), &outputLength);
    if (rc != 0) {
        SetError(error, "DSM provider transform failed");
        output.clear();
        return false;
    }
    output.resize(outputLength);
    if (error) error->clear();
    return true;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
