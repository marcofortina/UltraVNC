#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC Projects. All Rights Reserved.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${1:-/tmp/uvnc-winvnc-portable-core-build}"
INSTALL_PREFIX="${2:-}"

"${SCRIPT_DIR}/winvnc-portable-core-smoke.sh" "${BUILD_DIR}"

ctest --test-dir "${BUILD_DIR}" --output-on-failure -L 'winvnc-portable'
"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" \
    --allow-no-auth \
    --smoke-multi-update-test \
    --max-updates 3 \
    --fill-byte 119 \
    --pattern solid \
    --width 64 \
    --height 32 \
    --name winvnc-portable-closure

if [[ -n "${INSTALL_PREFIX}" ]]; then
    cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --help >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
        --allow-no-auth \
        --print-config \
        --pattern gradient-y \
        --fill-byte 42 \
        --width 64 \
        --height 32 \
        --name installed-winvnc-portable-closure >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
        --allow-no-auth \
        --smoke-multi-update-test \
        --max-updates 3 \
        --fill-byte 119 \
        --pattern solid \
        --width 64 \
        --height 32 \
        --name installed-winvnc-portable-closure
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_password_file" --help >/dev/null
    password_file="$(mktemp /tmp/uvnc-installed-password.XXXXXX)"
    rm -f "${password_file}"
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_password_file" --output "${password_file}" --password secret
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_password_file" --validate --output "${password_file}"
    rm -f "${password_file}"
fi
