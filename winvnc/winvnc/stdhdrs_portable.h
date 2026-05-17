// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef UVNC_WINVNC_STDHDRS_PORTABLE_H
#define UVNC_WINVNC_STDHDRS_PORTABLE_H

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif

using BYTE = unsigned char;
using BOOL = int;
using UINT = unsigned int;
using DWORD = std::uint32_t;
using ULONG = unsigned long;
using LONG = long;
using INT = int;
using WORD = unsigned short;
using LPCSTR = const char *;
using LPSTR = char *;
using PVOID = void *;
using HANDLE = void *;
using HDC = void *;
using HMODULE = void *;
using HWND = void *;
using HRESULT = long;
using VOID = void;

#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef MAXPATH
#define MAXPATH 256
#endif

struct POINT {
    LONG x;
    LONG y;
};

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct RGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
};

struct PALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
};

inline void ZeroMemory(void *ptr, std::size_t size)
{
    std::memset(ptr, 0, size);
}

inline void FillMemory(void *ptr, std::size_t size, int value)
{
    std::memset(ptr, value, size);
}

inline void CopyMemory(void *dest, const void *src, std::size_t size)
{
    std::memcpy(dest, src, size);
}

inline void SetRect(RECT *rect, LONG left, LONG top, LONG right, LONG bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

class VNCLog {
public:
    void Print(int, const char *, ...) {}
};

extern VNCLog vnclog;

#define LL_NONE 0
#define LL_STATE 0
#define LL_CLIENTS 1
#define LL_LOGSCREEN -1
#define LL_CONNERR 0
#define LL_SOCKERR 4
#define LL_INTERR 0
#define LL_INTWARN 8
#define LL_INTINFO 9
#define LL_SOCKINFO 10
#define LL_ALL 10
#define VNCLOG(s) (s)

#endif // UVNC_WINVNC_STDHDRS_PORTABLE_H
