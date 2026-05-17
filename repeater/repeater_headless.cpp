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

#include <signal.h>

int saved_mode2 = TRUE;
int saved_mode1 = FALSE;
int saved_keepalive = FALSE;

int saved_portA = 5901;
int saved_portB = 5500;
int saved_portHTTP = 0;
int saved_usecom = FALSE;

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

static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\n");
    printf("Options:\n");
    printf("  --viewer-port <port>   Viewer listen port, default 5901\n");
    printf("  --server-port <port>   Server listen port, default 5500\n");
    printf("  --mode1                Enable direct mode 1 connections\n");
    printf("  --no-mode2             Disable mode 2 server listener\n");
    printf("  --keepalive            Enable repeater keepalive messages\n");
    printf("  --help                 Show this help text\n");
}

static int parse_port(const char *value, int *port)
{
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (value == NULL || *value == '\0' || end == NULL || *end != '\0') return FALSE;
    if (parsed <= 0 || parsed > 65535) return FALSE;
    *port = (int)parsed;
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
        if (strcmp(argv[i], "--viewer-port") == 0 && i + 1 < argc) {
            if (!parse_port(argv[++i], &saved_portA)) return -1;
            continue;
        }
        if (strcmp(argv[i], "--server-port") == 0 && i + 1 < argc) {
            if (!parse_port(argv[++i], &saved_portB)) return -1;
            continue;
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return -1;
    }
    return 0;
}

char *lookup_comment(ULONG)
{
    return NULL;
}

void win_log(char *line)
{
    if (line != NULL) fprintf(stderr, "%s\n", line);
}

int main(int argc, char **argv)
{
    int parse_result = parse_args(argc, argv);
    if (parse_result > 0) return 0;
    if (parse_result < 0) {
        print_usage(argv[0]);
        return 1;
    }

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);

    return main_test();
}
