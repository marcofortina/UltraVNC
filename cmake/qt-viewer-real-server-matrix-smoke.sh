#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

if [[ "$#" -lt 2 ]]; then
  echo "usage: $0 <host> <port> [build-dir] [install-prefix] [password]" >&2
  echo "set UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT=1 to run the input/clipboard smoke" >&2
  exit 2
fi

host="$1"
port="$2"
build_dir="${3:-/tmp/uvnc-qt-viewer-real-server-build}"
install_prefix="${4:-/tmp/uvnc-qt-viewer-real-server-install}"
password="${5:-}"
allow_input="${UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT:-0}"

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=ON

if ! cmake --build "$build_dir" --target help | grep -q '^uvnc_qt_viewer:'; then
  echo "Skipping Qt viewer real-server matrix because Qt6 Widgets is not available." >&2
  exit 0
fi

cmake --build "$build_dir" --target uvnc_qt_viewer -j"$(nproc)"
cmake --install "$build_dir" --prefix "$install_prefix"

viewer_bin="$install_prefix/bin/uvnc_qt_viewer"
args=(--host "$host" --port "$port" --view-only --encodings raw,copyrect,newfbsize)
if [[ -n "$password" ]]; then
  args+=(--password "$password")
fi

echo "==> RFB handshake/update smoke"
timeout 15s "$viewer_bin" "${args[@]}" --connect-update-smoke

echo "==> Qt offscreen display smoke"
QT_QPA_PLATFORM=offscreen timeout 15s "$viewer_bin" "${args[@]}" --connect-display-smoke

if [[ "$allow_input" == "1" ]]; then
  echo "==> Persistent input/clipboard smoke"
  timeout 15s "$viewer_bin" "${args[@]}" \
    --clipboard-text "UltraVNC Qt viewer real-server smoke" \
    --persistent-input-smoke
else
  echo "Skipping input/clipboard smoke. Set UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT=1 to enable it." >&2
fi
