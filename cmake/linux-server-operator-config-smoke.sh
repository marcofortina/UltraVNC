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
BUILD_DIR="${1:-/tmp/uvnc-linux-server-operator-config-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-operator-config-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" --target uvnc_winvnc_memory_server -j"$(nproc)"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

BIN="${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server"
CONFIG_EXAMPLE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example"
DOC_RUNTIME="${INSTALL_PREFIX}/share/ultravnc/linux/native-linux-server-runtime.md"

test -x "${BIN}"
test -f "${CONFIG_EXAMPLE}"
test -f "${DOC_RUNTIME}"

grep -q '^# auth=vnc-password$' "${CONFIG_EXAMPLE}"
grep -q '^# password_file=/home/USER/.config/ultravnc/vnc-password$' "${CONFIG_EXAMPLE}"
grep -q '^# allow_unencrypted_public=false$' "${CONFIG_EXAMPLE}"

WORK_DIR="$(mktemp -d /tmp/uvnc-linux-server-operator-config.XXXXXX)"
cleanup() {
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

CONFIG_DIR="${WORK_DIR}/config"
mkdir -p "${CONFIG_DIR}"
chmod 700 "${CONFIG_DIR}"

PASSWORD_FILE="${CONFIG_DIR}/vnc-password"
printf '%s\n' 'secret1' >"${PASSWORD_FILE}"
chmod 600 "${PASSWORD_FILE}"

CONFIG_FILE="${CONFIG_DIR}/uvnc-winvnc-linux-server.conf"
sed \
  -e 's/^# auth=vnc-password$/auth=vnc-password/' \
  -e "s|^# password_file=/home/USER/.config/ultravnc/vnc-password$|password_file=${PASSWORD_FILE}|" \
  "${CONFIG_EXAMPLE}" >"${CONFIG_FILE}"
chmod 600 "${CONFIG_FILE}"

"${BIN}" --config "${CONFIG_FILE}" --validate-config 2>"${WORK_DIR}/validate.err"
grep -q 'VNCAuth protects the handshake' "${WORK_DIR}/validate.err"

"${BIN}" --config "${CONFIG_FILE}" --print-config >"${WORK_DIR}/print.out" 2>"${WORK_DIR}/print.err"
grep -q '^auth=vnc-password$' "${WORK_DIR}/print.out"
grep -q '^allow_unencrypted_public=no$' "${WORK_DIR}/print.out"
grep -q 'VNCAuth protects the handshake' "${WORK_DIR}/print.err"
if grep -qi 'secret1\|password_file=' "${WORK_DIR}/print.out"; then
  echo "operator print-config leaked password material or password-file path" >&2
  cat "${WORK_DIR}/print.out" >&2
  exit 1
fi

"${BIN}" --config "${CONFIG_FILE}" --print-admin-summary >"${WORK_DIR}/admin-summary.out" 2>"${WORK_DIR}/admin-summary.err"
grep -q '^linux_admin_equivalent=systemd-user-service$' "${WORK_DIR}/admin-summary.out"
grep -q '^windows_service_equivalent=uvnc-winvnc-memory-server.service$' "${WORK_DIR}/admin-summary.out"
grep -q '^windows_tray_ui_equivalent=qt-server-settings-live-admin$' "${WORK_DIR}/admin-summary.out"
grep -q '^windows_settings_ui_equivalent=config-file-plus-validate-config$' "${WORK_DIR}/admin-summary.out"
grep -q '^http_java_viewer=legacy-disabled$' "${WORK_DIR}/admin-summary.out"
grep -q '^dsm_plugin=native-provider-abi-available$' "${WORK_DIR}/admin-summary.out"
grep -q '^securevnc_plugin=native-dsm-provider-required$' "${WORK_DIR}/admin-summary.out"
grep -q '^mslogon_ii=server-external-helper-viewer-supported$' "${WORK_DIR}/admin-summary.out"
if grep -qi 'secret1\|password_file=' "${WORK_DIR}/admin-summary.out"; then
  echo "operator print-admin-summary leaked password material or password-file path" >&2
  cat "${WORK_DIR}/admin-summary.out" >&2
  exit 1
fi

LAN_CONFIG="${CONFIG_DIR}/uvnc-winvnc-linux-server-lan.conf"
cp "${CONFIG_FILE}" "${LAN_CONFIG}"
cat >>"${LAN_CONFIG}" <<'EOF_LAN'
bind_address=0.0.0.0
EOF_LAN
chmod 600 "${LAN_CONFIG}"
if "${BIN}" --config "${LAN_CONFIG}" --validate-config >"${WORK_DIR}/lan.out" 2>"${WORK_DIR}/lan.err"; then
  echo "operator LAN config without transport opt-in unexpectedly passed" >&2
  exit 1
fi
grep -q 'refusing VNCAuth on a non-loopback bind without transport encryption' "${WORK_DIR}/lan.err"

cat >>"${LAN_CONFIG}" <<'EOF_LAN_OPTIN'
allow_unencrypted_public=true
EOF_LAN_OPTIN
"${BIN}" --config "${LAN_CONFIG}" --validate-config >"${WORK_DIR}/lan-optin.out" 2>"${WORK_DIR}/lan-optin.err"
grep -q 'listening on 0.0.0.0' "${WORK_DIR}/lan-optin.err"
grep -q 'VNCAuth protects the handshake' "${WORK_DIR}/lan-optin.err"

echo "Linux server operator config smoke passed."
