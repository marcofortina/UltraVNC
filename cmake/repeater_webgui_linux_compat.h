// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef ULTRAVNC_REPEATER_WEBGUI_LINUX_COMPAT_H
#define ULTRAVNC_REPEATER_WEBGUI_LINUX_COMPAT_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef unsigned long DWORD;

#define _stricmp(s, t) strcasecmp((s), (t))
#define _strnicmp(s, t, n) strncasecmp((s), (t), (n))
#define GetLastError() errno

#ifndef strncpy_s
#define strncpy_s(dest, destsz, src, count) do { \
    size_t uvnc_copy_len = (count); \
    if (uvnc_copy_len >= (destsz)) uvnc_copy_len = (destsz) - 1; \
    strncpy((dest), (src), uvnc_copy_len); \
    (dest)[uvnc_copy_len] = '\0'; \
} while (0)
#endif

#endif
