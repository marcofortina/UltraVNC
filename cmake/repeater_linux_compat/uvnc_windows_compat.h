// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef ULTRAVNC_REPEATER_LINUX_WINDOWS_COMPAT_H
#define ULTRAVNC_REPEATER_LINUX_WINDOWS_COMPAT_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#ifndef SD_SEND
#define SD_SEND SHUT_WR
#endif
#ifndef WSAECONNRESET
#define WSAECONNRESET ECONNRESET
#endif

typedef int BOOL;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef void *LPVOID;
typedef int SOCKET;
typedef fd_set FD_SET;
typedef unsigned long ULONG;
typedef char TCHAR;
typedef char *LPTSTR;
typedef const char *LPCSTR;
typedef pthread_t *HANDLE;
typedef struct _WSADATA {
    WORD wVersion;
    WORD wHighVersion;
} WSADATA;

typedef struct _SYSTEMTIME {
    int wYear;
    int wMonth;
    int wDay;
    int wHour;
    int wMinute;
    int wSecond;
} SYSTEMTIME;

#define TEXT(value) value
static inline int uvnc_closesocket(SOCKET socket)
{
    if (socket != INVALID_SOCKET) shutdown(socket, SHUT_RDWR);
    return close(socket);
}

#define closesocket(socket) uvnc_closesocket(socket)
#define Sleep(milliseconds) usleep((milliseconds) * 1000)
#define WSAGetLastError() errno
#define timeGetTime() GetTickCount()
#define MAKEWORD(low, high) ((WORD)((((WORD)(high)) << 8) | ((WORD)(low))))
#define WSAStartup(version, data) (0)
#define WSACleanup() (0)
#define _tcschr strchr
#define _tcslen strlen
#define _stscanf_s sscanf
#define _tcsncpy_s(dest, destsz, src, count) strncpy_s((dest), (destsz), (src), (count))

static inline DWORD GetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)((ts.tv_sec * 1000u) + (ts.tv_nsec / 1000000u));
}

static inline void GetLocalTime(SYSTEMTIME *st)
{
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    st->wYear = local.tm_year + 1900;
    st->wMonth = local.tm_mon + 1;
    st->wDay = local.tm_mday;
    st->wHour = local.tm_hour;
    st->wMinute = local.tm_min;
    st->wSecond = local.tm_sec;
}

static inline HANDLE CreateThread(void *, size_t, DWORD (WINAPI *start)(LPVOID), LPVOID arg, DWORD, DWORD *threadId)
{
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    if (thread == NULL) return NULL;
    if (pthread_create(thread, NULL, (void *(*)(void *))start, arg) != 0) {
        free(thread);
        return NULL;
    }
    if (threadId != NULL) *threadId = 0;
    return thread;
}

static inline DWORD WaitForSingleObject(HANDLE handle, DWORD)
{
    if (handle == NULL) return 1;
    return pthread_join(*handle, NULL) == 0 ? 0 : 1;
}

static inline BOOL CloseHandle(HANDLE handle)
{
    if (handle != NULL) free(handle);
    return TRUE;
}

static inline int uvnc_accept(SOCKET socket, struct sockaddr *addr, int *addrlen)
{
    socklen_t len = (addrlen != NULL) ? (socklen_t)*addrlen : 0;
    int rc = accept(socket, addr, addrlen != NULL ? &len : NULL);
    if (addrlen != NULL) *addrlen = (int)len;
    return rc;
}

#define accept(socket, addr, addrlen) uvnc_accept((socket), (addr), (addrlen))

static inline char *uvnc_strtok_s(char *str, const char *delim, char **context)
{
    return strtok_r(str, delim, context);
}

#define strtok_s(str, delim, context) uvnc_strtok_s((str), (delim), (context))

static inline BOOL GetModuleFileName(void *, char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) return FALSE;
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len < 0) return FALSE;
    buffer[len] = '\0';
    return TRUE;
}

static inline int uvnc_strcpy_s(char *dest, size_t destsz, const char *src)
{
    if (dest == NULL || destsz == 0) return EINVAL;
    if (src == NULL) src = "";
    snprintf(dest, destsz, "%s", src);
    return 0;
}

#ifdef __cplusplus
template <size_t N>
static inline int strcpy_s(char (&dest)[N], const char *src)
{
    return uvnc_strcpy_s(dest, N, src);
}

static inline int strcpy_s(char *dest, size_t destsz, const char *src)
{
    return uvnc_strcpy_s(dest, destsz, src);
}
#else
#define strcpy_s(dest, destsz, src) uvnc_strcpy_s((dest), (destsz), (src))
#endif

static inline int uvnc_strcat_s(char *dest, size_t destsz, const char *src)
{
    size_t len;
    if (dest == NULL || destsz == 0) return EINVAL;
    if (src == NULL) src = "";
    len = strlen(dest);
    if (len >= destsz) return EINVAL;
    snprintf(dest + len, destsz - len, "%s", src);
    return 0;
}

#ifdef __cplusplus
template <size_t N>
static inline int strcat_s(char (&dest)[N], const char *src)
{
    return uvnc_strcat_s(dest, N, src);
}

static inline int strcat_s(char *dest, size_t destsz, const char *src)
{
    return uvnc_strcat_s(dest, destsz, src);
}
#else
#define strcat_s(dest, destsz, src) uvnc_strcat_s((dest), (destsz), (src))
#endif

static inline int uvnc_strncpy_s(char *dest, size_t destsz, const char *src, size_t count)
{
    size_t len;
    if (dest == NULL || destsz == 0) return EINVAL;
    if (src == NULL) src = "";
    len = count;
    if (len >= destsz) len = destsz - 1;
    strncpy(dest, src, len);
    dest[len] = '\0';
    return 0;
}

#ifdef __cplusplus
template <size_t N>
static inline int strncpy_s(char (&dest)[N], const char *src, size_t count)
{
    return uvnc_strncpy_s(dest, N, src, count);
}

static inline int strncpy_s(char *dest, size_t destsz, const char *src, size_t count)
{
    return uvnc_strncpy_s(dest, destsz, src, count);
}
#else
#define strncpy_s(dest, destsz, src, count) uvnc_strncpy_s((dest), (destsz), (src), (count))
#endif

static inline int uvnc_vsnprintf_s(char *buffer, size_t size, const char *fmt, va_list args)
{
    if (buffer == NULL || size == 0 || fmt == NULL) return EINVAL;
    return vsnprintf(buffer, size, fmt, args);
}

#ifdef __cplusplus
template <size_t N>
static inline int sprintf_s(char (&buffer)[N], const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int rc = uvnc_vsnprintf_s(buffer, N, fmt, args);
    va_end(args);
    return rc;
}

static inline int sprintf_s(char *buffer, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int rc = uvnc_vsnprintf_s(buffer, size, fmt, args);
    va_end(args);
    return rc;
}

template <size_t N>
static inline int vsprintf_s(char (&buffer)[N], const char *fmt, va_list args)
{
    return uvnc_vsnprintf_s(buffer, N, fmt, args);
}

static inline int vsprintf_s(char *buffer, size_t size, const char *fmt, va_list args)
{
    return uvnc_vsnprintf_s(buffer, size, fmt, args);
}
#else
#define sprintf_s(buffer, size, ...) snprintf((buffer), (size), __VA_ARGS__)
#define vsprintf_s(buffer, size, fmt, args) vsnprintf((buffer), (size), (fmt), (args))
#endif

#define _itoa_s(value, buffer, radix) snprintf((buffer), sizeof(buffer), (radix) == 16 ? "%x" : "%d", (value))
#define _ltoa_s(value, buffer, radix) snprintf((buffer), sizeof(buffer), (radix) == 16 ? "%lx" : "%ld", (long)(value))

#endif
