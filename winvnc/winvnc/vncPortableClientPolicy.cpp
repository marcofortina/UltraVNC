// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableClientPolicy.h"

namespace uvnc {
namespace winvnc {
namespace portable {

ClientConnectionPolicy::ClientConnectionPolicy(unsigned int maxSharedClients)
    : maxSharedClients_(maxSharedClients == 0 ? 1 : maxSharedClients),
      activeClients_(0),
      exclusiveClientActive_(false)
{
}

bool ClientConnectionPolicy::CanAccept(bool sharedClientRequested, std::string *reason) const
{
    if (exclusiveClientActive_) {
        if (reason) *reason = "exclusive client is already connected";
        return false;
    }
    if (!sharedClientRequested && activeClients_ != 0) {
        if (reason) *reason = "exclusive client requested while shared clients are connected";
        return false;
    }
    if (sharedClientRequested && activeClients_ >= maxSharedClients_) {
        if (reason) *reason = "maximum shared clients reached";
        return false;
    }
    if (reason) reason->clear();
    return true;
}

bool ClientConnectionPolicy::RegisterClient(bool sharedClientRequested, std::string *reason)
{
    if (!CanAccept(sharedClientRequested, reason)) {
        return false;
    }
    activeClients_ += 1;
    exclusiveClientActive_ = !sharedClientRequested;
    if (reason) reason->clear();
    return true;
}

void ClientConnectionPolicy::UnregisterClient(bool)
{
    if (activeClients_ > 0) {
        activeClients_ -= 1;
    }
    if (activeClients_ == 0) {
        exclusiveClientActive_ = false;
    }
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
