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
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${1:-/tmp/uvnc-memory-server-rfb-build}"
INSTALL_PREFIX="${2:-}"

cmake -S "${REPO_ROOT}/cmake" -B "${BUILD_DIR}" -G Ninja \
    -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
    -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
    -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
    -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"

memory_rfb_regex='winvnc_(portable_(rfb|memory_server|server_config|framebuffer_pattern|framebuffer_gradient)|memory_server_)'
expected_memory_rfb_tests=30
actual_memory_rfb_tests=$(ctest --test-dir "${BUILD_DIR}" -N -R "${memory_rfb_regex}" | sed -n 's/^Total Tests: //p')
if [[ -z "${actual_memory_rfb_tests}" || "${actual_memory_rfb_tests}" -lt "${expected_memory_rfb_tests}" ]]; then
    echo "Expected at least ${expected_memory_rfb_tests} Memory server/RFB tests, got ${actual_memory_rfb_tests:-0}" >&2
    exit 1
fi

ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "${memory_rfb_regex}"

"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" --help >/dev/null
"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" \
    --print-config \
    --pattern checker \
    --fill-byte 85 \
    --width 64 \
    --height 32 \
    --name memory-rfb-closure >/dev/null
"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" \
    --smoke-test \
    --pattern checker \
    --fill-byte 85 \
    --width 64 \
    --height 32 \
    --name memory-rfb-handshake-closure
"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" \
    --smoke-update-test \
    --pattern gradient-x \
    --fill-byte 42 \
    --width 64 \
    --height 32 \
    --name memory-rfb-update-closure
"${BUILD_DIR}/winvnc_memory_server/uvnc_winvnc_memory_server" \
    --smoke-multi-update-test \
    --max-updates 3 \
    --pattern solid \
    --fill-byte 119 \
    --width 64 \
    --height 32 \
    --name memory-rfb-multi-update-closure

if [[ -n "${INSTALL_PREFIX}" ]]; then
    cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --help >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
        --print-config \
        --pattern gradient-y \
        --fill-byte 42 \
        --width 64 \
        --height 32 \
        --name installed-memory-rfb-closure >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
        --smoke-multi-update-test \
        --max-updates 3 \
        --pattern solid \
        --fill-byte 119 \
        --width 64 \
        --height 32 \
        --name installed-memory-rfb-closure
fi
