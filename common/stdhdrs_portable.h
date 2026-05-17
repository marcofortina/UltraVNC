// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef UVNC_COMMON_STDHDRS_PORTABLE_H
#define UVNC_COMMON_STDHDRS_PORTABLE_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using BYTE = unsigned char;
using BOOL = int;
using UINT = unsigned int;
using DWORD = std::uint32_t;
using ULONG = unsigned long;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#include "rfb.h"

#endif // UVNC_COMMON_STDHDRS_PORTABLE_H
