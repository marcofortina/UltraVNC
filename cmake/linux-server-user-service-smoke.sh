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
BUILD_DIR="${1:-/tmp/uvnc-linux-server-user-service-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-user-service-install}"
UNIT_NAME="uvnc-winvnc-memory-server.service"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

SERVICE_FILE="${INSTALL_PREFIX}/share/ultravnc/linux/${UNIT_NAME}"
CONFIG_EXAMPLE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example"
ENV_EXAMPLE="${INSTALL_PREFIX}/share/ultravnc/linux/uvnc-winvnc-memory-server.env.example"
BIN="${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server"

test -x "${BIN}"
test -f "${SERVICE_FILE}"
test -f "${CONFIG_EXAMPLE}"
test -f "${ENV_EXAMPLE}"
grep -q -- '--serve-forever' "${SERVICE_FILE}"
grep -q -- '--pid-file %t/uvnc-winvnc-memory-server.pid' "${SERVICE_FILE}"
grep -q -- '--status-file %t/uvnc-winvnc-memory-server.status' "${SERVICE_FILE}"
grep -q -- '--log-file %t/uvnc-winvnc-memory-server.log' "${SERVICE_FILE}"

if command -v systemd-analyze >/dev/null 2>&1; then
  systemd-analyze verify "${SERVICE_FILE}" >/dev/null
else
  echo "Skipping systemd-analyze verify because systemd-analyze is unavailable."
fi

if [[ "${UVNC_RUN_SYSTEMD_USER_SERVICE:-0}" != "1" ]]; then
  echo "Skipping live systemd user-service smoke because UVNC_RUN_SYSTEMD_USER_SERVICE=1 is not set."
  echo "The non-invasive template and install checks passed."
  exit 0
fi

if ! command -v systemctl >/dev/null 2>&1; then
  echo "systemctl is required for live user-service smoke." >&2
  exit 1
fi

if [[ -z "${XDG_RUNTIME_DIR:-}" ]]; then
  echo "XDG_RUNTIME_DIR is required for systemd --user runtime state." >&2
  exit 1
fi

if ! systemctl --user show-environment >/dev/null 2>&1; then
  echo "systemd --user is not available for this session." >&2
  exit 1
fi

USER_CONFIG_DIR="${HOME}/.config/ultravnc"
USER_CONFIG_FILE="${USER_CONFIG_DIR}/uvnc-winvnc-linux-server.conf"
USER_ENV_FILE="${USER_CONFIG_DIR}/uvnc-winvnc-memory-server.env"

if [[ -e "${USER_CONFIG_FILE}" && "${UVNC_OVERWRITE_USER_SERVICE_CONFIG:-0}" != "1" ]]; then
  echo "Refusing to overwrite ${USER_CONFIG_FILE}; set UVNC_OVERWRITE_USER_SERVICE_CONFIG=1 for this opt-in smoke." >&2
  exit 1
fi
if [[ -e "${USER_ENV_FILE}" && "${UVNC_OVERWRITE_USER_SERVICE_CONFIG:-0}" != "1" ]]; then
  echo "Refusing to overwrite ${USER_ENV_FILE}; set UVNC_OVERWRITE_USER_SERVICE_CONFIG=1 for this opt-in smoke." >&2
  exit 1
fi

mkdir -p "${USER_CONFIG_DIR}"
USER_PASSWORD_FILE="${USER_CONFIG_DIR}/vnc-password"
printf '%s\n' 'secret1' > "${USER_PASSWORD_FILE}"
chmod 600 "${USER_PASSWORD_FILE}"

cat > "${USER_CONFIG_FILE}" <<EOF_CONFIG
bind_address=127.0.0.1
port=0
name=uvnc-linux-user-service-smoke
capture_backend=memory
input_backend=none
auth=vnc-password
password_file=${USER_PASSWORD_FILE}
max_updates=1
serve_forever=true
EOF_CONFIG
chmod 600 "${USER_CONFIG_FILE}"
cp "${ENV_EXAMPLE}" "${USER_ENV_FILE}"

cleanup() {
  systemctl --user stop "${UNIT_NAME}" >/dev/null 2>&1 || true
  systemctl --user disable "${UNIT_NAME}" >/dev/null 2>&1 || true
  systemctl --user reset-failed "${UNIT_NAME}" >/dev/null 2>&1 || true
  rm -f "${USER_CONFIG_FILE}" "${USER_ENV_FILE}" "${USER_PASSWORD_FILE:-}"
}
trap cleanup EXIT

systemctl --user link "${SERVICE_FILE}" >/dev/null
systemctl --user daemon-reload
systemctl --user start "${UNIT_NAME}"

for _ in $(seq 1 100); do
  if systemctl --user is-active --quiet "${UNIT_NAME}"; then
    break
  fi
  sleep 0.1
done

systemctl --user is-active --quiet "${UNIT_NAME}"
test -s "${XDG_RUNTIME_DIR}/uvnc-winvnc-memory-server.pid"
test -s "${XDG_RUNTIME_DIR}/uvnc-winvnc-memory-server.status"
grep -q '^listening 127\.0\.0\.1:' "${XDG_RUNTIME_DIR}/uvnc-winvnc-memory-server.status"

systemctl --user stop "${UNIT_NAME}"
for _ in $(seq 1 100); do
  if ! systemctl --user is-active --quiet "${UNIT_NAME}"; then
    break
  fi
  sleep 0.1
done

if systemctl --user is-active --quiet "${UNIT_NAME}"; then
  echo "user service did not stop cleanly" >&2
  exit 1
fi

echo "Live systemd user-service smoke passed."
