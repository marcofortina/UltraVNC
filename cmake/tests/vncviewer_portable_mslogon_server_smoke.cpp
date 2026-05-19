// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableViewerConfig.h"
#include "vncPortableViewerSession.h"

#include <cassert>
#include <fstream>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::vncviewer::portable;
using namespace uvnc::winvnc::portable;

int main()
{
    const std::string helper = std::string("/tmp/uvnc-mslogon-server-helper-") + std::to_string(getpid()) + ".sh";
    {
        std::ofstream out(helper.c_str(), std::ios::trunc);
        out << "#!/usr/bin/env sh\n"
            << "[ \"$UVNC_AUTH_METHOD\" = \"mslogon-ii\" ] || exit 2\n"
            << "[ \"$UVNC_AUTH_USERNAME\" = \"alice\" ] || exit 3\n"
            << "[ \"$UVNC_AUTH_PASSWORD\" = \"secret\" ] || exit 4\n"
            << "exit 0\n";
    }
    assert(chmod(helper.c_str(), 0700) == 0);

    ServerConfig serverConfig;
    serverConfig.SetBindAddress("127.0.0.1");
    serverConfig.SetPort(0);
    serverConfig.SetAllowNoAuth(false);
    serverConfig.SetAuthMode(ServerAuthMode::MsLogonII);
    serverConfig.SetAuthHelperPath(helper);
    serverConfig.SetSize(8, 6);

    MemoryServer server;
    assert(server.Start(serverConfig));

    bool served = false;
    std::thread worker([&]() {
        served = server.ServeOne();
    });

    ViewerConfig viewerConfig;
    viewerConfig.SetHost("127.0.0.1");
    viewerConfig.SetPort(server.Port());
    viewerConfig.SetSecurityExtension(ViewerSecurityExtensionMode::MsLogon);
    viewerConfig.SetUsername("alice");
    viewerConfig.SetPassword("secret");
    viewerConfig.SetRequestUpdate(false);

    ViewerSession viewer;
    ViewerSessionResult result;
    std::string error;
    assert(viewer.RunHandshake(viewerConfig, result, &error));
    assert(result.width == 8);
    assert(result.height == 6);

    worker.join();
    server.Stop();
    unlink(helper.c_str());
    assert(served);
    return 0;
}
