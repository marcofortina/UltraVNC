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
BUILD_DIR="${1:-/tmp/uvnc-linux-server-secure-runtime-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-secure-runtime-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" --target \
  uvnc_winvnc_memory_server \
  winvnc_portable_rfb_vncauth_session_smoke \
  -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'winvnc_memory_server_security_config|winvnc_portable_rfb_vncauth_session_smoke|winvnc_memory_server_config_file'
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

BIN="${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server"
test -x "${BIN}"

WORK_DIR="$(mktemp -d /tmp/uvnc-linux-server-secure-runtime.XXXXXX)"
cleanup() {
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

PASSWORD_FILE="${WORK_DIR}/vnc-password"
printf '%s\n' 'secret1' >"${PASSWORD_FILE}"
chmod 600 "${PASSWORD_FILE}"

BAD_PASSWORD_FILE="${WORK_DIR}/bad-vnc-password"
printf '%s\n' 'secret1' >"${BAD_PASSWORD_FILE}"
chmod 0640 "${BAD_PASSWORD_FILE}"
if "${BIN}" --password-file "${BAD_PASSWORD_FILE}" --validate-config >"${WORK_DIR}/bad.out" 2>"${WORK_DIR}/bad.err"; then
  echo "expected private password-file validation failure" >&2
  exit 1
fi
grep -q 'password file must not be accessible by group/other' "${WORK_DIR}/bad.err"

VNCAUTH_VALIDATE_ERR="${WORK_DIR}/vncauth-validate.err"
"${BIN}" \
  --auth vnc-password \
  --password-file "${PASSWORD_FILE}" \
  --bind-address 127.0.0.1 \
  --capture-backend memory \
  --input-backend none \
  --validate-config 2>"${VNCAUTH_VALIDATE_ERR}"
grep -q 'VNCAuth protects the handshake' "${VNCAUTH_VALIDATE_ERR}"

if "${BIN}" \
  --auth vnc-password \
  --password-file "${PASSWORD_FILE}" \
  --bind-address 0.0.0.0 \
  --capture-backend memory \
  --input-backend none \
  --validate-config >"${WORK_DIR}/public-vncauth.out" 2>"${WORK_DIR}/public-vncauth.err"; then
  echo "expected non-loopback VNCAuth without transport opt-in to fail" >&2
  exit 1
fi
grep -q 'refusing VNCAuth on a non-loopback bind without transport encryption' "${WORK_DIR}/public-vncauth.err"

"${BIN}" \
  --auth vnc-password \
  --password-file "${PASSWORD_FILE}" \
  --bind-address 0.0.0.0 \
  --allow-unencrypted-public \
  --capture-backend memory \
  --input-backend none \
  --validate-config >"${WORK_DIR}/public-vncauth-optin.out" 2>"${WORK_DIR}/public-vncauth-optin.err"
grep -q 'listening on 0.0.0.0' "${WORK_DIR}/public-vncauth-optin.err"
grep -q 'VNCAuth protects the handshake' "${WORK_DIR}/public-vncauth-optin.err"

CONFIG_OUT="${WORK_DIR}/print-config.out"
CONFIG_ERR="${WORK_DIR}/print-config.err"
"${BIN}" \
  --password-file "${PASSWORD_FILE}" \
  --bind-address 127.0.0.1 \
  --capture-backend memory \
  --input-backend none \
  --print-config >"${CONFIG_OUT}" 2>"${CONFIG_ERR}"
grep -q '^auth=vnc-password$' "${CONFIG_OUT}"
grep -q 'VNCAuth protects the handshake' "${CONFIG_ERR}"
if grep -qi 'secret1\|password_file=' "${CONFIG_OUT}"; then
  echo "print-config must not expose password material or password-file paths" >&2
  cat "${CONFIG_OUT}" >&2
  exit 1
fi

"${BIN}" \
  --password-file "${PASSWORD_FILE}" \
  --smoke-test \
  --width 64 \
  --height 32 \
  --name secure-vncauth-handshake-smoke 2>"${WORK_DIR}/vncauth-handshake.err"

"${BIN}" \
  --password-file "${PASSWORD_FILE}" \
  --smoke-update-test \
  --width 64 \
  --height 32 \
  --name secure-vncauth-update-smoke 2>"${WORK_DIR}/vncauth-update.err"

"${BIN}" \
  --password-file "${PASSWORD_FILE}" \
  --smoke-multi-update-test \
  --max-updates 3 \
  --width 64 \
  --height 32 \
  --name secure-vncauth-multi-update-smoke 2>"${WORK_DIR}/vncauth-multi-update.err"

if [[ "${UVNC_RUN_SECURE_X11_SERVER:-0}" != "1" ]]; then
  echo "Skipping secure live X11 VNCAuth smoke because UVNC_RUN_SECURE_X11_SERVER=1 is not set."
  exit 0
fi

if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY is required for secure live X11 VNCAuth smoke." >&2
  exit 1
fi

if [[ "${XDG_SESSION_TYPE:-}" != "x11" ]]; then
  echo "XDG_SESSION_TYPE must be x11 for secure live X11 VNCAuth smoke; got ${XDG_SESSION_TYPE:-unset}." >&2
  exit 1
fi

case "${DISPLAY}" in
  localhost:*|127.0.0.1:*|::1:*)
    echo "DISPLAY=${DISPLAY} looks like SSH X forwarding, not a local desktop capture target." >&2
    exit 1
    ;;
esac

"${BIN}" \
  --password-file "${PASSWORD_FILE}" \
  --capture-backend x11 \
  --input-backend none \
  --smoke-x11-update-test \
  --name secure-x11-vncauth-smoke 2>"${WORK_DIR}/secure-x11-vncauth.err"

echo "Secure Linux server runtime smoke passed."
