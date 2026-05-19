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
BUILD_DIR="${1:-/tmp/uvnc-linux-server-negative-runtime-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-negative-runtime-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'linux_server_integration|winvnc_memory_server_.*(config|x11|xtest)|winvnc_linux_(capture|x11|input|xtest)'
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

BIN="${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server"
test -x "${BIN}"

expect_fail() {
  local description="$1"
  shift
  if "$@" >/tmp/uvnc-negative-runtime.out 2>/tmp/uvnc-negative-runtime.err; then
    echo "Expected failure but command succeeded: ${description}" >&2
    cat /tmp/uvnc-negative-runtime.out >&2 || true
    cat /tmp/uvnc-negative-runtime.err >&2 || true
    exit 1
  fi
}

WORK_DIR="$(mktemp -d /tmp/uvnc-linux-server-negative-runtime.XXXXXX)"
cleanup() {
  if [[ -n "${PORT_HOLDER:-}" ]] && kill -0 "${PORT_HOLDER}" 2>/dev/null; then
    kill "${PORT_HOLDER}" 2>/dev/null || true
    wait "${PORT_HOLDER}" 2>/dev/null || true
  fi
  rm -rf "${WORK_DIR}"
  rm -f /tmp/uvnc-negative-runtime.out /tmp/uvnc-negative-runtime.err
}
trap cleanup EXIT

BAD_CONFIG="${WORK_DIR}/bad.conf"
cat > "${BAD_CONFIG}" <<'EOF_CONFIG'
bind_address=127.0.0.1
unknown_key=value
EOF_CONFIG
chmod 600 "${BAD_CONFIG}"
expect_fail "unknown config key" "${BIN}" --config "${BAD_CONFIG}" --validate-config
grep -q 'unknown config key' /tmp/uvnc-negative-runtime.err

EMPTY_CONFIG="${WORK_DIR}/empty-value.conf"
cat > "${EMPTY_CONFIG}" <<'EOF_CONFIG'
name=
EOF_CONFIG
chmod 600 "${EMPTY_CONFIG}"
expect_fail "empty config value" "${BIN}" --config "${EMPTY_CONFIG}" --validate-config
grep -q 'empty value for config key' /tmp/uvnc-negative-runtime.err

expect_fail "invalid capture backend" "${BIN}" --allow-no-auth --capture-backend definitely-not-a-backend --validate-config
grep -q 'invalid --capture-backend' /tmp/uvnc-negative-runtime.err

expect_fail "invalid input backend" "${BIN}" --allow-no-auth --input-backend definitely-not-a-backend --validate-config
grep -q 'invalid --input-backend' /tmp/uvnc-negative-runtime.err

expect_fail "missing log parent validation" "${BIN}" --allow-no-auth --log-file "${WORK_DIR}/missing-dir/server.log" --validate-config
grep -q 'log-file parent directory does not exist' /tmp/uvnc-negative-runtime.err

ln -s "${WORK_DIR}/target.log" "${WORK_DIR}/server.log"
expect_fail "symlink log path validation" "${BIN}" --allow-no-auth --log-file "${WORK_DIR}/server.log" --validate-config
grep -q 'log-file path must not be a symlink' /tmp/uvnc-negative-runtime.err

: >"${WORK_DIR}/status-file"
chmod 0666 "${WORK_DIR}/status-file"
expect_fail "world-writable status path validation" "${BIN}" --allow-no-auth --status-file "${WORK_DIR}/status-file" --validate-config
grep -q 'status-file file must not be group/world writable' /tmp/uvnc-negative-runtime.err

PASSWORD_FILE="${WORK_DIR}/vnc-password"
printf '%s\n' 'secret1' >"${PASSWORD_FILE}"
chmod 600 "${PASSWORD_FILE}"
expect_fail "public VNCAuth without transport opt-in" "${BIN}" --password-file "${PASSWORD_FILE}" --bind-address 0.0.0.0 --validate-config
grep -q 'refusing VNCAuth on a non-loopback bind without transport encryption' /tmp/uvnc-negative-runtime.err

PORT_HOLDER=""
PORT_FILE="${WORK_DIR}/held-port"
python3 - "${PORT_FILE}" <<'PY' &
import pathlib
import socket
import sys
import time

path = pathlib.Path(sys.argv[1])
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 0)
sock.bind(("127.0.0.1", 0))
sock.listen(1)
path.write_text(str(sock.getsockname()[1]))
try:
    time.sleep(30)
finally:
    sock.close()
PY
PORT_HOLDER=$!
for _ in $(seq 1 50); do
  [[ -s "${PORT_FILE}" ]] && break
  sleep 0.1
done
HELD_PORT="$(cat "${PORT_FILE}")"
expect_fail "occupied TCP port" "${BIN}" --allow-no-auth --bind-address 127.0.0.1 --port "${HELD_PORT}" --serve-updates --max-updates 1
kill "${PORT_HOLDER}" 2>/dev/null || true
wait "${PORT_HOLDER}" 2>/dev/null || true
PORT_HOLDER=""
grep -q 'failed to start memory server' /tmp/uvnc-negative-runtime.err

if DISPLAY= "${BIN}" --allow-no-auth --validate-config --capture-backend x11 >/tmp/uvnc-negative-runtime.out 2>/tmp/uvnc-negative-runtime.err; then
  echo "explicit X11 backend unexpectedly validated with DISPLAY unset" >&2
  exit 1
fi
grep -q 'capture backend is not available: x11' /tmp/uvnc-negative-runtime.err

if DISPLAY= "${BIN}" --allow-no-auth --validate-config --input-backend xtest >/tmp/uvnc-negative-runtime.out 2>/tmp/uvnc-negative-runtime.err; then
  echo "explicit XTest backend unexpectedly validated with DISPLAY unset" >&2
  exit 1
fi
grep -q 'input backend is not available: xtest' /tmp/uvnc-negative-runtime.err

echo "Linux server negative runtime smoke passed."
