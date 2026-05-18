#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

REAL_RUNTIME_HELPER="${1:?real runtime helper path required}"
USER_SERVICE_HELPER="${2:?user service helper path required}"

bash -n "${REAL_RUNTIME_HELPER}"
bash -n "${USER_SERVICE_HELPER}"

grep -q 'UVNC_RUN_REAL_X11_SERVER=1' "${REAL_RUNTIME_HELPER}"
grep -q 'DISPLAY is required' "${REAL_RUNTIME_HELPER}"
grep -q 'XDG_SESSION_TYPE must be x11' "${REAL_RUNTIME_HELPER}"
grep -q 'SSH X forwarding' "${REAL_RUNTIME_HELPER}"
grep -q 'pid-file' "${REAL_RUNTIME_HELPER}"
grep -q 'status-file' "${REAL_RUNTIME_HELPER}"
grep -q 'log-file' "${REAL_RUNTIME_HELPER}"
grep -q 'RFB 003.008' "${REAL_RUNTIME_HELPER}"

grep -q 'UVNC_RUN_SYSTEMD_USER_SERVICE=1' "${USER_SERVICE_HELPER}"
grep -q 'systemd-analyze verify' "${USER_SERVICE_HELPER}"
grep -q 'systemctl --user start' "${USER_SERVICE_HELPER}"
grep -q 'Refusing to overwrite' "${USER_SERVICE_HELPER}"
grep -q 'uvnc-winvnc-memory-server.status' "${USER_SERVICE_HELPER}"
