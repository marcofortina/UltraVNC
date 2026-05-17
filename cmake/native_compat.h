// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef ULTRAVNC_NATIVE_COMPAT_H
#define ULTRAVNC_NATIVE_COMPAT_H

#ifndef _WIN32

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

typedef int64_t LONGLONG;
#define __int64 int64_t

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#define strncat_s(dst, src, count) strncat((dst), (src), (count))
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), __VA_ARGS__)

static inline int uvnc_strerror_s(char* buffer, size_t bufferSize, int err)
{
    snprintf(buffer, bufferSize, "%s", strerror(err));
    return 0;
}

#define strerror_s(buffer, bufferSize, err) uvnc_strerror_s((buffer), (bufferSize), (err))

static inline int64_t uvnc_passed_usecs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

#define Passedusecs() uvnc_passed_usecs()
#define __debugbreak() abort()

#endif // _WIN32

#endif // ULTRAVNC_NATIVE_COMPAT_H
