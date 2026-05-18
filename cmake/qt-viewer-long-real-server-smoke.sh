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
  echo "usage: $0 <host> <port> [build-dir] [install-prefix] [password] [iterations]" >&2
  exit 2
fi

host="$1"
port="$2"
build_dir="${3:-/tmp/uvnc-qt-viewer-long-real-server-build}"
install_prefix="${4:-/tmp/uvnc-qt-viewer-long-real-server-install}"
password="${5:-}"
iterations="${6:-10}"

"$(dirname "$0")/qt-viewer-known-server-smoke.sh" \
  "$host" \
  "$port" \
  "$build_dir" \
  "$install_prefix" \
  "$password"

viewer_bin="$install_prefix/bin/uvnc_qt_viewer"
args=(--host "$host" --port "$port" --view-only --encodings raw,copyrect,newfbsize)
if [[ -n "$password" ]]; then
  args+=(--password "$password")
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
