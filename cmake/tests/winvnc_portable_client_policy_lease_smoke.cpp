// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableClientPolicy.h"

#include <cassert>
#include <string>

using uvnc::winvnc::portable::ClientConnectionLease;
using uvnc::winvnc::portable::ClientConnectionPolicy;

int main()
{
    ClientConnectionPolicy policy(2);
    {
        ClientConnectionLease first(&policy, true);
        std::string reason;
        assert(first.Acquire(&reason));
        assert(reason.empty());
        assert(first.Active());
        assert(policy.ActiveClients() == 1);
        {
            ClientConnectionLease second(&policy, true);
            assert(second.Acquire(&reason));
            assert(policy.ActiveClients() == 2);
            ClientConnectionLease third(&policy, true);
            assert(!third.Acquire(&reason));
            assert(reason == "maximum shared clients reached");
        }
        assert(policy.ActiveClients() == 1);
    }
    assert(policy.ActiveClients() == 0);

    ClientConnectionLease exclusive(&policy, false);
    assert(exclusive.Acquire(nullptr));
    assert(policy.HasExclusiveClient());
    exclusive.Release();
    assert(!policy.HasExclusiveClient());
    return 0;
}
