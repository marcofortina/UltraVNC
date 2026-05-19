#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${1:-/tmp/uvnc-native-linux-final-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-native-linux-final-install}"
MATRIX_FILE="${3:-}"
SERVER_HOST="${UVNC_EXTERNAL_VIEWER_MATRIX_HOST:-127.0.0.1}"
SERVER_PORT="${UVNC_EXTERNAL_VIEWER_MATRIX_PORT:-5901}"
PASSWORD_FILE="${UVNC_EXTERNAL_VIEWER_MATRIX_PASSWORD_FILE:-}"

"${SCRIPT_DIR}/native-linux-ci-gate-smoke.sh" "${BUILD_ROOT}/ci" "${INSTALL_PREFIX}"

if [[ -z "${MATRIX_FILE}" ]]; then
  cat >&2 <<'EOF_MISSING'
Native Linux local CI gate passed, but final readiness is not complete.
Missing required external viewer matrix argument.

Run with a real matrix file, for example:
  cmake/native-linux-final-readiness-smoke.sh \
    /tmp/uvnc-native-linux-final-build \
    /tmp/uvnc-native-linux-final-install \
    docs/examples/native-linux-server-external-viewer-matrix.example
EOF_MISSING
  exit 3
fi

"${SCRIPT_DIR}/linux-server-external-viewer-matrix-check.sh" "${MATRIX_FILE}"

if [[ -n "${PASSWORD_FILE}" ]]; then
  "${SCRIPT_DIR}/linux-server-external-viewer-matrix-smoke.sh" \
    "${MATRIX_FILE}" "${SERVER_HOST}" "${SERVER_PORT}" "${PASSWORD_FILE}"
else
  echo "External viewer matrix syntax passed. Set UVNC_EXTERNAL_VIEWER_MATRIX_PASSWORD_FILE to execute ready rows." >&2
  exit 3
fi

echo "Native Linux final readiness smoke passed."
