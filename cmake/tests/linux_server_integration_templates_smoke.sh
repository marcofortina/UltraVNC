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

if [[ ! -f "${SERVICE_FILE}" ]]; then
  echo "missing generated service file: ${SERVICE_FILE}" >&2
  exit 1
fi
if [[ ! -f "${ENV_EXAMPLE}" ]]; then
  echo "missing env example: ${ENV_EXAMPLE}" >&2
  exit 1
fi

grep -q 'ExecStart=.*/uvnc_winvnc_memory_server' "${SERVICE_FILE}"
grep -q -- '--capture-backend ${UVNC_CAPTURE_BACKEND}' "${SERVICE_FILE}"
grep -q -- '--input-backend ${UVNC_INPUT_BACKEND}' "${SERVICE_FILE}"
grep -q '^UVNC_BIND_ADDRESS=127\.0\.0\.1$' "${ENV_EXAMPLE}"
grep -q '^UVNC_CAPTURE_BACKEND=auto$' "${ENV_EXAMPLE}"
grep -q '^UVNC_INPUT_BACKEND=none$' "${ENV_EXAMPLE}"
