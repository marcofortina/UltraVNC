// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(4, 4);

    MemoryServer server;
    assert(server.Start(config));

    bool accepted = true;
    assert(server.TryServeOneUpdates(1, nullptr, 10, accepted));
    assert(!accepted);

    server.Stop();
    return 0;
}
