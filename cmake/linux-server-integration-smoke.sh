#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

BUILD_DIR="${1:-/tmp/uvnc-linux-server-integration-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-integration-install}"

cmake -S cmake -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -R 'linux_server_integration|winvnc_memory_server_.*(capture|input|x11|pipewire|xtest)|winvnc_linux_(capture|x11|pipewire|input|xtest)'
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

SERVICE_FILE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-memory-server.service"
ENV_FILE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-memory-server.env.example"
CONFIG_FILE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example"
INTEGRATION_DOC="${INSTALL_PREFIX}/share/ultravnc/linux/native-linux-server-backend-integration.md"

test -f "${SERVICE_FILE}"
test -f "${ENV_FILE}"
test -f "${CONFIG_FILE}"
test -f "${INTEGRATION_DOC}"
grep -q 'uvnc_winvnc_memory_server' "${SERVICE_FILE}"
grep -q "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" "${SERVICE_FILE}"
grep -q -- '--log-file %t/uvnc-winvnc-memory-server.log' "${SERVICE_FILE}"
grep -q '^capture_backend=auto$' "${CONFIG_FILE}"
grep -q '^input_backend=none$' "${CONFIG_FILE}"
grep -q '^# auth=vnc-password$' "${CONFIG_FILE}"
grep -q '^# password_file=/home/USER/.config/ultravnc/vnc-password$' "${CONFIG_FILE}"
grep -q '^# auth=none$' "${CONFIG_FILE}"
grep -q '^# allow_no_auth=true$' "${CONFIG_FILE}"
grep -q 'Manual X11 validation' "${INTEGRATION_DOC}"
grep -q 'Manual PipeWire/XDG portal validation' "${INTEGRATION_DOC}"
grep -q 'Manual XTest validation' "${INTEGRATION_DOC}"

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --allow-no-auth --validate-config --config "${CONFIG_FILE}" --capture-backend memory --input-backend none
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --allow-no-auth --print-config --config "${CONFIG_FILE}" --capture-backend auto --input-backend auto >/dev/null
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-pipewire-availability-test >/dev/null
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-xtest-availability-test >/dev/null

"$(dirname "$0")/x11-capture-smoke.sh" "${BUILD_DIR}" "${INSTALL_PREFIX}"
"$(dirname "$0")/pipewire-capture-smoke.sh" "${BUILD_DIR}" "${INSTALL_PREFIX}"
"$(dirname "$0")/linux-input-smoke.sh" "${BUILD_DIR}" "${INSTALL_PREFIX}"
