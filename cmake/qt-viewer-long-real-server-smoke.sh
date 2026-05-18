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
  echo "usage: $0 <host> <port> [build-dir] [install-prefix] [password-or-password-file] [iterations]" >&2
  exit 2
fi

host="$1"
port="$2"
build_dir="${3:-/tmp/uvnc-qt-viewer-long-real-server-build}"
install_prefix="${4:-/tmp/uvnc-qt-viewer-long-real-server-install}"
password="${5:-}"
iterations="${6:-10}"
password_file="${UVNC_VIEWER_PASSWORD_FILE:-}"
password_temp=""

"$(dirname "$0")/qt-viewer-known-server-smoke.sh" \
  "$host" \
  "$port" \
  "$build_dir" \
  "$install_prefix" \
  "$password"

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
args=(--host "$host" --port "$port" --view-only --encodings "${UVNC_VIEWER_REAL_SERVER_ENCODINGS:-raw,copyrect,hextile,zlib,zrle,rre,corre,newfbsize}")
if [[ -n "$password_file" ]]; then
  args+=(--password-file "$password_file")
fi

for i in $(seq 1 "$iterations"); do
  echo "==> long real-server update iteration ${i}/${iterations}"
  timeout 10s "$viewer_bin" "${args[@]}" --connect-update-smoke
  QT_QPA_PLATFORM=offscreen timeout 10s "$viewer_bin" "${args[@]}" --connect-display-smoke >/tmp/uvnc-qt-viewer-long-display-${i}.log 2>&1 || {
    cat /tmp/uvnc-qt-viewer-long-display-${i}.log >&2
    exit 1
  }
  grep -q '^displayed ' /tmp/uvnc-qt-viewer-long-display-${i}.log || {
    cat /tmp/uvnc-qt-viewer-long-display-${i}.log >&2
    exit 1
  }
  rm -f /tmp/uvnc-qt-viewer-long-display-${i}.log
  sleep 0.2
done
