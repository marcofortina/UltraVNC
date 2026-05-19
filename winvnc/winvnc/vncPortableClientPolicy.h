// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_CLIENT_POLICY_H
#define UVNC_WINVNC_PORTABLE_CLIENT_POLICY_H

#include <mutex>
#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class ClientConnectionPolicy {
public:
    explicit ClientConnectionPolicy(unsigned int maxSharedClients = 8);

    bool CanAccept(bool sharedClientRequested, std::string *reason = nullptr) const;
    bool RegisterClient(bool sharedClientRequested, std::string *reason = nullptr);
    void UnregisterClient(bool sharedClientRequested);

    unsigned int ActiveClients() const;
    bool HasExclusiveClient() const;

private:
    unsigned int maxSharedClients_;
    mutable std::mutex mutex_;
    unsigned int activeClients_;
    bool exclusiveClientActive_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_CLIENT_POLICY_H
