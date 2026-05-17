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
BUILD_DIR="${1:-/tmp/uvnc-linux-build}"
INSTALL_PREFIX="${2:-}"

cmake -S "${REPO_ROOT}/cmake" -B "${BUILD_DIR}" -G Ninja \
    -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
    -DULTRAVNC_BUILD_WINDOWS_APPS=OFF
cmake --build "${BUILD_DIR}" -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
"${BUILD_DIR}/repeater_headless/uvnc_repeater_headless" --smoke-test --quiet

CONFIG_FILE="${BUILD_DIR}/uvnc-repeater-headless-smoke.conf"
cat >"${CONFIG_FILE}" <<EOF
mode1=true
mode2=false
viewer-port=5901
server-port=5500
bind-address=127.0.0.1
log-dir=${BUILD_DIR}
quiet=true
EOF
"${BUILD_DIR}/repeater_headless/uvnc_repeater_headless" --config "${CONFIG_FILE}" --validate-config --quiet

if [[ -n "${INSTALL_PREFIX}" ]]; then
    cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --help >/dev/null
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --mode1 --no-mode2 --validate-config --quiet
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --config "${CONFIG_FILE}" --validate-config --quiet
    "${INSTALL_PREFIX}/bin/uvnc_repeater_headless" --smoke-test --quiet
fi
