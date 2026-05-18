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
  echo "usage: $0 <host> <port> [build-dir] [install-prefix] [password-or-password-file]" >&2
  exit 2
fi

host="$1"
port="$2"
build_dir="${3:-/tmp/uvnc-qt-viewer-known-server-build}"
install_prefix="${4:-/tmp/uvnc-qt-viewer-known-server-install}"
password="${5:-}"
password_file="${UVNC_VIEWER_PASSWORD_FILE:-}"
password_temp=""

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=ON

if ! cmake --build "$build_dir" --target help | grep -q '^uvnc_qt_viewer:'; then
  echo "Skipping Qt viewer known-server smoke because Qt6 Widgets is not available." >&2
  exit 0
fi

cmake --build "$build_dir" --target uvnc_qt_viewer -j"$(nproc)"
cmake --install "$build_dir" --prefix "$install_prefix"

viewer_bin="$install_prefix/bin/uvnc_qt_viewer"
cleanup_password_file() {
  if [[ -n "$password_temp" ]]; then
    rm -f "$password_temp"
  fi
}
trap cleanup_password_file EXIT
if [[ -z "$password_file" && -n "${UVNC_VIEWER_PASSWORD:-}" ]]; then
  password_temp="$(mktemp)"
  chmod 600 "$password_temp"
  printf %s "$UVNC_VIEWER_PASSWORD" >"$password_temp"
  password_file="$password_temp"
elif [[ -z "$password_file" && -n "$password" ]]; then
  if [[ -r "$password" ]]; then
    password_file="$password"
  else
    password_temp="$(mktemp)"
    chmod 600 "$password_temp"
    printf %s "$password" >"$password_temp"
    password_file="$password_temp"
  fi
fi

args=(--host "$host" --port "$port" --view-only --encodings "${UVNC_VIEWER_REAL_SERVER_ENCODINGS:-raw,copyrect,hextile,zlib,zrle,tight,rre,corre,newfbsize}")
if [[ -n "$password_file" ]]; then
  args+=(--password-file "$password_file")
fi

timeout 10s "$viewer_bin" "${args[@]}" --connect-update-smoke
QT_QPA_PLATFORM=offscreen timeout 10s "$viewer_bin" "${args[@]}" --connect-display-smoke

if [[ "${UVNC_VIEWER_KNOWN_SERVER_RUN_INPUT:-0}" == "1" ]]; then
  timeout 10s "$viewer_bin" "${args[@]}" \
    --clipboard-text "UltraVNC Qt viewer known-server smoke" \
    --persistent-input-smoke
fi
