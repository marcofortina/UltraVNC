#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${1:-/tmp/uvnc-repeater-linux-build}"
INSTALL_PREFIX="${2:-}"

cmake -S "${REPO_ROOT}/cmake" -B "${BUILD_DIR}" -G Ninja \
    -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
    -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
    -DULTRAVNC_BUILD_REPEATER_HEADLESS=ON \
    -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=OFF

cmake --build "${BUILD_DIR}" -j"$(nproc)"

expected_repeater_tests=16
actual_repeater_tests=$(ctest --test-dir "${BUILD_DIR}" -N -R '^uvnc_repeater_headless' | sed -n 's/^Total Tests: //p')
if [[ -z "${actual_repeater_tests}" || "${actual_repeater_tests}" -lt "${expected_repeater_tests}" ]]; then
    echo "Expected at least ${expected_repeater_tests} repeater tests, got ${actual_repeater_tests:-0}" >&2
    exit 1
fi

ctest --test-dir "${BUILD_DIR}" --output-on-failure -R '^uvnc_repeater_headless'
"${BUILD_DIR}/repeater_headless/uvnc_repeater_headless" --smoke-test --quiet

config_file="${BUILD_DIR}/uvnc-repeater-linux-closure.conf"
cat >"${config_file}" <<EOF_CONFIG
mode1=true
mode2=false
viewer-port=5901
server-port=5500
bind-address=127.0.0.1
log-dir=${BUILD_DIR}
quiet=true
EOF_CONFIG
"${BUILD_DIR}/repeater_headless/uvnc_repeater_headless" --config "${config_file}" --validate-config --quiet

if [[ -n "${INSTALL_PREFIX}" ]]; then
    cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --help >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --mode1 --no-mode2 --validate-config --quiet
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --config "${config_file}" --validate-config --quiet
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --smoke-test --quiet
fi
