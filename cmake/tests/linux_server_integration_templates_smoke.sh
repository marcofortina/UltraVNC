#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

SERVICE_FILE="$1"
ENV_EXAMPLE="$2"
CONFIG_EXAMPLE="$3"

if [[ ! -f "${SERVICE_FILE}" ]]; then
  echo "missing generated service file: ${SERVICE_FILE}" >&2
  exit 1
fi
if [[ ! -f "${ENV_EXAMPLE}" ]]; then
  echo "missing env example: ${ENV_EXAMPLE}" >&2
  exit 1
fi
if [[ ! -f "${CONFIG_EXAMPLE}" ]]; then
  echo "missing config example: ${CONFIG_EXAMPLE}" >&2
  exit 1
fi

grep -q 'ExecStart=.*/uvnc_winvnc_memory_server' "${SERVICE_FILE}"
grep -q -- '--config %h/.config/ultravnc/uvnc-winvnc-linux-server.conf' "${SERVICE_FILE}"
grep -q -- '--serve-forever' "${SERVICE_FILE}"
grep -q -- '--pid-file ${UVNC_PID_FILE}' "${SERVICE_FILE}"
grep -q -- '--status-file ${UVNC_STATUS_FILE}' "${SERVICE_FILE}"
grep -q '^UVNC_PID_FILE=%t/uvnc-winvnc-memory-server.pid$' "${ENV_EXAMPLE}"
grep -q '^UVNC_STATUS_FILE=%t/uvnc-winvnc-memory-server.status$' "${ENV_EXAMPLE}"
grep -q '^bind_address=127\.0\.0\.1$' "${CONFIG_EXAMPLE}"
grep -q '^capture_backend=auto$' "${CONFIG_EXAMPLE}"
grep -q '^input_backend=none$' "${CONFIG_EXAMPLE}"
