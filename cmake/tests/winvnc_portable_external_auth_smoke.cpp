// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExternalAuth.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string helper = std::string("/tmp/uvnc-auth-helper-") + std::to_string(getpid()) + ".sh";
    {
        std::ofstream out(helper.c_str(), std::ios::trunc);
        out << "#!/usr/bin/env sh\n"
            << "[ \"$UVNC_AUTH_METHOD\" = \"mslogon-ii\" ] || exit 2\n"
            << "[ \"$UVNC_AUTH_USERNAME\" = \"alice\" ] || exit 3\n"
            << "[ \"$UVNC_AUTH_PASSWORD\" = \"secret\" ] || exit 4\n"
            << "exit 0\n";
    }
    assert(chmod(helper.c_str(), 0700) == 0);

    ExternalAuthRequest request;
    request.method = "mslogon-ii";
    request.username = "alice";
    request.password = "secret";
    std::string error;
    assert(RunExternalAuthHelper(helper, request, &error));

    request.password = "wrong";
    assert(!RunExternalAuthHelper(helper, request, &error));
    assert(error.find("rejected") != std::string::npos);

    unlink(helper.c_str());
    return 0;
}
