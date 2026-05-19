// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExternalAuth.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

extern char **environ;

namespace uvnc {
namespace winvnc {
namespace portable {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) *error = message;
}

} // namespace

bool RunExternalAuthHelper(const std::string& helperPath,
                           const ExternalAuthRequest& request,
                           std::string *error)
{
    if (helperPath.empty() || helperPath[0] != '/') {
        SetError(error, "external auth helper path must be absolute");
        return false;
    }
    if (request.username.empty()) {
        SetError(error, "external auth request username must not be empty");
        return false;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        SetError(error, std::string("fork external auth helper failed: ") + std::strerror(errno));
        return false;
    }
    if (pid == 0) {
        setenv("UVNC_AUTH_USERNAME", request.username.c_str(), 1);
        setenv("UVNC_AUTH_PASSWORD", request.password.c_str(), 1);
        setenv("UVNC_AUTH_METHOD", request.method.c_str(), 1);
        execl(helperPath.c_str(), helperPath.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        SetError(error, std::string("wait external auth helper failed: ") + std::strerror(errno));
        return false;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (error) error->clear();
        return true;
    }
    if (WIFEXITED(status)) {
        SetError(error, "external auth helper rejected credentials");
    } else if (WIFSIGNALED(status)) {
        SetError(error, "external auth helper terminated by signal");
    } else {
        SetError(error, "external auth helper did not exit cleanly");
    }
    return false;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
