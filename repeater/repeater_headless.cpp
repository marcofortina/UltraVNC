// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#include "repeater.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>

int saved_mode2 = TRUE;
int saved_mode1 = FALSE;
int saved_keepalive = FALSE;

int saved_portA = 5901;
int saved_portB = 5500;
int saved_portHTTP = 0;
int saved_usecom = FALSE;
int saved_quiet = FALSE;
unsigned long saved_bind_address = htonl(INADDR_ANY);
char saved_log_dir[MAX_PATH] = "";

int saved_allow = FALSE;
int saved_refuse = FALSE;
int saved_refuse2 = FALSE;
char saved_sample1[1024] = "";
char saved_sample2[1024] = "";
char saved_sample3[1024] = "";
char saved_password[64] = "";

char temp1[50][25];
char temp2[50][16];
char temp3[50][16];
int rule1 = 0;
int rule2 = 0;
int rule3 = 0;

mystruct Servers[MAX_LIST];
mystruct Viewers[MAX_LIST];
mycomstruct comment[MAX_LIST * 2];

int notstopped = TRUE;
int notwebstopped = TRUE;

extern int main_test();

static void request_shutdown(int)
{
    notstopped = FALSE;
    notwebstopped = FALSE;
}

static int saved_smoke_test = FALSE;
static int saved_validate_config = FALSE;

static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\n");
    printf("Options:\n");
    printf("  --viewer-port <port>   Viewer listen port, default 5901\n");
    printf("  --server-port <port>   Server listen port, default 5500\n");
    printf("  --bind-address <ipv4> Bind listeners to an IPv4 address, default 0.0.0.0\n");
    printf("  --log-dir <path>      Write repeater access logs under this directory\n");
    printf("  --mode1                Enable direct mode 1 connections\n");
    printf("  --no-mode2             Disable mode 2 server listener\n");
    printf("  --keepalive            Enable repeater keepalive messages\n");
    printf("  --smoke-test           Start listeners on free ports and verify they accept connections\n");
    printf("  --validate-config      Validate options and exit without starting listeners\n");
    printf("  --quiet                Suppress normal repeater status output\n");
    printf("  --help                 Show this help text\n");
}

static int parse_port(const char *value, int *port)
{
    char *end = NULL;
    long parsed;

    if (value == NULL || *value == '\0' || port == NULL) return FALSE;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || end == NULL || *end != '\0') return FALSE;
    if (parsed <= 0 || parsed > 65535 || parsed > INT_MAX) return FALSE;

    *port = (int)parsed;
    return TRUE;
}

static int parse_log_dir(const char *value)
{
    if (value == NULL || *value == '\0') return FALSE;
    if (strlen(value) >= sizeof(saved_log_dir)) return FALSE;

    strcpy_s(saved_log_dir, sizeof(saved_log_dir), value);
    return TRUE;
}

static int parse_bind_address(const char *value)
{
    struct in_addr parsed;

    if (value == NULL || *value == '\0') return FALSE;
    if (inet_pton(AF_INET, value, &parsed) != 1) return FALSE;

    saved_bind_address = parsed.s_addr;
    return TRUE;
}

static int parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        }
        if (strcmp(argv[i], "--mode1") == 0) {
            saved_mode1 = TRUE;
            continue;
        }
        if (strcmp(argv[i], "--no-mode2") == 0) {
            saved_mode2 = FALSE;
            continue;
        }
        if (strcmp(argv[i], "--keepalive") == 0) {
            saved_keepalive = TRUE;
            continue;
        }
        if (strcmp(argv[i], "--smoke-test") == 0) {
            saved_smoke_test = TRUE;
            continue;
        }
        if (strcmp(argv[i], "--validate-config") == 0) {
            saved_validate_config = TRUE;
            continue;
        }
        if (strcmp(argv[i], "--quiet") == 0) {
            saved_quiet = TRUE;
            continue;
        }
        if (strcmp(argv[i], "--viewer-port") == 0) {
            if (i + 1 >= argc || !parse_port(argv[++i], &saved_portA)) {
                fprintf(stderr, "Invalid --viewer-port value\n");
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--server-port") == 0) {
            if (i + 1 >= argc || !parse_port(argv[++i], &saved_portB)) {
                fprintf(stderr, "Invalid --server-port value\n");
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--bind-address") == 0) {
            if (i + 1 >= argc || !parse_bind_address(argv[++i])) {
                fprintf(stderr, "Invalid --bind-address value\n");
                return -1;
            }
            continue;
        }
        if (strcmp(argv[i], "--log-dir") == 0) {
            if (i + 1 >= argc || !parse_log_dir(argv[++i])) {
                fprintf(stderr, "Invalid --log-dir value\n");
                return -1;
            }
            continue;
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return -1;
    }
    return 0;
}


static int validate_runtime_config(void)
{
    if (!saved_mode1 && !saved_mode2) {
        fprintf(stderr, "Invalid configuration: at least one repeater mode must be enabled\n");
        return FALSE;
    }
    if (saved_mode2 && saved_portA == saved_portB) {
        fprintf(stderr, "Invalid configuration: viewer and server ports must be different when mode 2 is enabled\n");
        return FALSE;
    }
    return TRUE;
}

static int find_free_loopback_port(void)
{
    SOCKET probe = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);

    if (probe == INVALID_SOCKET) return -1;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(probe, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR ||
        getsockname(probe, (struct sockaddr *)&address, &address_len) == SOCKET_ERROR) {
        closesocket(probe);
        return -1;
    }

    int port = ntohs(address.sin_port);
    closesocket(probe);
    return port;
}

static int find_distinct_free_loopback_port(int excluded_port)
{
    for (int attempt = 0; attempt < 16; attempt++) {
        int port = find_free_loopback_port();
        if (port > 0 && port != excluded_port) return port;
    }
    return -1;
}

static int wait_for_loopback_port(int port, int timeout_ms)
{
    int elapsed_ms = 0;

    while (elapsed_ms < timeout_ms) {
        SOCKET probe = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address;

        if (probe == INVALID_SOCKET) return FALSE;

        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons((u_short)port);

        if (connect(probe, (struct sockaddr *)&address, sizeof(address)) == 0) {
            closesocket(probe);
            return TRUE;
        }

        closesocket(probe);
        Sleep(50);
        elapsed_ms += 50;
    }

    return FALSE;
}

static DWORD WINAPI run_repeater_for_smoke(LPVOID)
{
    return (DWORD)main_test();
}

static int run_smoke_test(void)
{
    int viewer_port = find_free_loopback_port();
    int server_port = find_distinct_free_loopback_port(viewer_port);
    DWORD thread_id = 0;
    HANDLE repeater_thread;
    int viewer_ready;
    int server_ready;

    if (viewer_port <= 0 || server_port <= 0 || viewer_port == server_port) {
        fprintf(stderr, "Unable to allocate local smoke-test ports\n");
        return 1;
    }

    saved_portA = viewer_port;
    saved_portB = server_port;
    saved_bind_address = htonl(INADDR_LOOPBACK);
    saved_mode2 = TRUE;
    saved_mode1 = FALSE;
    saved_keepalive = FALSE;
    saved_quiet = TRUE;
    notstopped = TRUE;
    notwebstopped = TRUE;

    repeater_thread = CreateThread(NULL, 0, run_repeater_for_smoke, NULL, 0, &thread_id);
    if (repeater_thread == NULL) {
        fprintf(stderr, "Unable to start smoke-test repeater thread\n");
        return 1;
    }

    viewer_ready = wait_for_loopback_port(viewer_port, 5000);
    server_ready = wait_for_loopback_port(server_port, 5000);

    request_shutdown(0);
    WaitForSingleObject(repeater_thread, 5000);
    CloseHandle(repeater_thread);

    if (!viewer_ready || !server_ready) {
        fprintf(stderr, "Smoke test failed: viewer_ready=%d server_ready=%d\n", viewer_ready, server_ready);
        return 1;
    }

    return 0;
}

char *lookup_comment(ULONG)
{
    return NULL;
}

void win_log(char *line)
{
    if (saved_quiet) return;
    if (line != NULL) fprintf(stderr, "%s\n", line);
}

int main(int argc, char **argv)
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    int parse_result = parse_args(argc, argv);
    if (parse_result > 0) return 0;
    if (parse_result < 0) {
        print_usage(argv[0]);
        return 1;
    }
    if (saved_smoke_test) return run_smoke_test();
    if (!validate_runtime_config()) return 1;
    if (saved_validate_config) return 0;

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);

    return main_test();
}
