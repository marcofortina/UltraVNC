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
BUILD_DIR="${1:-/tmp/uvnc-linux-server-real-runtime-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-real-runtime-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'linux_server_integration|winvnc_memory_server_.*(x11|xtest|config)|winvnc_linux_(capture|x11|input|xtest)'
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

BIN="${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server"
test -x "${BIN}"

"${BIN}" --smoke-x11-availability-test
"${BIN}" --smoke-xtest-availability-test
"${BIN}" --validate-config --capture-backend memory --input-backend none
"${BIN}" --print-config --capture-backend auto --input-backend auto >/dev/null

if [[ "${UVNC_RUN_REAL_X11_SERVER:-0}" != "1" ]]; then
  echo "Skipping real X11 server runtime smoke because UVNC_RUN_REAL_X11_SERVER=1 is not set."
  echo "Set it only from a real local X11 graphical session, not from SSH X forwarding."
  exit 0
fi

if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY is required for the real X11 server runtime smoke." >&2
  exit 1
fi

if [[ "${XDG_SESSION_TYPE:-}" != "x11" ]]; then
  echo "XDG_SESSION_TYPE must be x11 for the real X11 server runtime smoke; got ${XDG_SESSION_TYPE:-unset}." >&2
  exit 1
fi

case "${DISPLAY}" in
  localhost:*|127.0.0.1:*|::1:*)
    echo "DISPLAY=${DISPLAY} looks like SSH X forwarding, not a local desktop capture target." >&2
    exit 1
    ;;
esac

"${BIN}" --validate-config --capture-backend x11 --input-backend none

RUNTIME_DIR="$(mktemp -d /tmp/uvnc-linux-server-real-runtime.XXXXXX)"
CONFIG_FILE="${RUNTIME_DIR}/uvnc-winvnc-linux-server.conf"
PID_FILE="${RUNTIME_DIR}/uvnc-winvnc-memory-server.pid"
STATUS_FILE="${RUNTIME_DIR}/uvnc-winvnc-memory-server.status"
LOG_FILE="${RUNTIME_DIR}/uvnc-winvnc-memory-server.log"
SERVER_PID=""

cleanup() {
  if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
    kill -TERM "${SERVER_PID}" 2>/dev/null || true
    wait "${SERVER_PID}" 2>/dev/null || true
  fi
  rm -rf "${RUNTIME_DIR}"
}
trap cleanup EXIT

cat > "${CONFIG_FILE}" <<EOF_CONFIG
bind_address=127.0.0.1
port=0
name=uvnc-linux-real-runtime-smoke
capture_backend=x11
input_backend=none
max_updates=1
serve_forever=true
EOF_CONFIG

"${BIN}" \
  --config "${CONFIG_FILE}" \
  --serve-forever \
  --pid-file "${PID_FILE}" \
  --status-file "${STATUS_FILE}" \
  --log-file "${LOG_FILE}" &
SERVER_PID="$!"

for _ in $(seq 1 100); do
  if [[ -s "${STATUS_FILE}" ]] && grep -q '^listening ' "${STATUS_FILE}"; then
    break
  fi
  if ! kill -0 "${SERVER_PID}" 2>/dev/null; then
    echo "server exited before listening" >&2
    cat "${LOG_FILE}" >&2 2>/dev/null || true
    exit 1
  fi
  sleep 0.1
done

test -s "${PID_FILE}"
grep -q "^${SERVER_PID}$" "${PID_FILE}"
grep -q '^listening 127\.0\.0\.1:' "${STATUS_FILE}"
LISTEN_PORT="$(sed -n 's/^listening 127\.0\.0\.1:\([0-9][0-9]*\)$/\1/p' "${STATUS_FILE}" | tail -n 1)"
if [[ -z "${LISTEN_PORT}" || "${LISTEN_PORT}" == "0" ]]; then
  echo "could not parse listening port from ${STATUS_FILE}" >&2
  cat "${STATUS_FILE}" >&2
  exit 1
fi

python3 - "${LISTEN_PORT}" <<'PY'
import socket
import struct
import sys

port = int(sys.argv[1])


def read_exact(sock, size):
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise RuntimeError(f"short read: wanted {size}, got {len(data)}")
        data += chunk
    return data

with socket.create_connection(("127.0.0.1", port), timeout=10) as sock:
    sock.settimeout(10)
    version = read_exact(sock, 12)
    if not version.startswith(b"RFB "):
        raise RuntimeError(f"unexpected protocol version: {version!r}")
    sock.sendall(b"RFB 003.008\n")
    security_count = read_exact(sock, 1)[0]
    security_types = read_exact(sock, security_count)
    if 1 not in security_types:
        raise RuntimeError(f"NoAuth security type not offered: {security_types!r}")
    sock.sendall(b"\x01")
    auth_result = struct.unpack("!I", read_exact(sock, 4))[0]
    if auth_result != 0:
        raise RuntimeError(f"authentication failed with result {auth_result}")
    sock.sendall(b"\x01")
    server_init = read_exact(sock, 24)
    width, height = struct.unpack("!HH", server_init[:4])
    bits_per_pixel = server_init[4]
    name_length = struct.unpack("!I", server_init[20:24])[0]
    name = read_exact(sock, name_length).decode("utf-8", "replace")
    if width == 0 or height == 0:
        raise RuntimeError(f"invalid framebuffer size {width}x{height}")
    if name != "uvnc-linux-real-runtime-smoke":
        raise RuntimeError(f"unexpected desktop name: {name!r}")
    sock.sendall(struct.pack("!BBHHHH", 3, 0, 0, 0, width, height))
    update = read_exact(sock, 4)
    if update[0] != 0:
        raise RuntimeError(f"unexpected framebuffer update message type {update[0]}")
    rects = struct.unpack("!H", update[2:4])[0]
    if rects != 1:
        raise RuntimeError(f"expected one raw rectangle, got {rects}")
    rect_header = read_exact(sock, 12)
    rx, ry, rw, rh, encoding = struct.unpack("!HHHHi", rect_header)
    if (rx, ry, rw, rh) != (0, 0, width, height):
        raise RuntimeError(f"unexpected rectangle {(rx, ry, rw, rh)} for framebuffer {(width, height)}")
    if encoding != 0:
        raise RuntimeError(f"expected raw encoding, got {encoding}")
    pixel_bytes = width * height * max(1, bits_per_pixel // 8)
    read_exact(sock, pixel_bytes)
PY

kill -TERM "${SERVER_PID}"
wait "${SERVER_PID}"
SERVER_PID=""

grep -q '^stopped$' "${STATUS_FILE}"
test ! -e "${PID_FILE}"
grep -q 'listening on 127\.0\.0\.1:' "${LOG_FILE}"

echo "Real X11 server runtime smoke passed."
