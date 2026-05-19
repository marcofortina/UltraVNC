#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

build_dir="${1:-/tmp/uvnc-qt-viewer-persistent-input-build}"
install_prefix="${2:-/tmp/uvnc-qt-viewer-persistent-input-install}"

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=ON \
  -DULTRAVNC_BUILD_QT_VIEWER=ON

if ! cmake --build "$build_dir" --target help | grep -q '^uvnc_qt_viewer:'; then
  echo "Skipping Qt viewer persistent input smoke because Qt6 Widgets is not available." >&2
  exit 0
fi

cmake --build "$build_dir" --target uvnc_winvnc_memory_server uvnc_qt_viewer -j"$(nproc)"
cmake --install "$build_dir" --prefix "$install_prefix"

port="$(python3 - <<'PY'
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('127.0.0.1', 0))
print(s.getsockname()[1])
s.close()
PY
)"

server_bin="$install_prefix/bin/uvnc_winvnc_memory_server"
viewer_bin="$install_prefix/bin/uvnc_qt_viewer"
server_log="$(mktemp)"
viewer_output="$(mktemp)"
server_pid=""
cleanup() {
  if [[ -n "$server_pid" ]]; then
    kill "$server_pid" 2>/dev/null || true
  fi
  rm -f "$server_log" "$viewer_output"
}
trap cleanup EXIT

"$server_bin" \
  --allow-no-auth \
  --bind-address 127.0.0.1 \
  --port "$port" \
  --width 64 \
  --height 48 \
  --name qt-viewer-persistent-input-smoke \
  --capture-backend memory \
  --fill-byte 120 \
  --serve-updates \
  --max-updates 1 >"$server_log" 2>&1 &
server_pid="$!"

server_ready=0
for _ in $(seq 1 100); do
  if grep -q "listening on 127.0.0.1:$port" "$server_log"; then
    server_ready=1
    break
  fi
  if ! kill -0 "$server_pid" 2>/dev/null; then
    echo "Qt viewer persistent input smoke server exited before listening." >&2
    cat "$server_log" >&2
    exit 1
  fi
  sleep 0.05
done

if [[ "$server_ready" != "1" ]]; then
  echo "Qt viewer persistent input smoke server did not become ready." >&2
  cat "$server_log" >&2
  exit 1
fi

if ! timeout 10s "$viewer_bin" --host 127.0.0.1 --port "$port" --persistent-input-smoke >"$viewer_output" 2>&1; then
  echo "Qt viewer failed to complete the persistent input smoke." >&2
  cat "$viewer_output" >&2
  cat "$server_log" >&2
  exit 1
fi
cat "$viewer_output"
grep -q 'persistent-input 64x48 name="qt-viewer-persistent-input-smoke" update=64x48 bytes=12288 clipboard=sent' "$viewer_output" || {
  echo "Qt viewer persistent input smoke output did not match expectations." >&2
  cat "$viewer_output" >&2
  cat "$server_log" >&2
  exit 1
}

server_done=0
for _ in $(seq 1 100); do
  if ! kill -0 "$server_pid" 2>/dev/null; then
    server_done=1
    break
  fi
  sleep 0.05
done

if [[ "$server_done" != "1" ]]; then
  echo "Qt viewer persistent input smoke server did not exit after the viewer completed." >&2
  cat "$server_log" >&2
  exit 1
fi
wait "$server_pid"
trap - EXIT
rm -f "$server_log" "$viewer_output"
