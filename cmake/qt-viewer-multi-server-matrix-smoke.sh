#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

if [[ "$#" -lt 1 ]]; then
  echo "usage: $0 <matrix-file> [build-dir] [install-prefix]" >&2
  echo "matrix columns: label|host|port|password-or-file|encodings|allow-input" >&2
  exit 2
fi

matrix_file="$1"
build_dir="${2:-/tmp/uvnc-qt-viewer-multi-server-build}"
install_prefix="${3:-/tmp/uvnc-qt-viewer-multi-server-install}"

if [[ ! -r "$matrix_file" ]]; then
  echo "matrix file is not readable: $matrix_file" >&2
  exit 2
fi

line_no=0
while IFS= read -r line || [[ -n "$line" ]]; do
  line_no=$((line_no + 1))
  [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
  IFS='|' read -r label host port password encodings allow_input extra <<<"$line"
  if [[ -n "${extra:-}" || -z "${label:-}" || -z "${host:-}" || -z "${port:-}" ]]; then
    echo "invalid matrix line ${line_no}: ${line}" >&2
    exit 2
  fi
  encodings="${encodings:-raw,copyrect,hextile,zlib,zrle,rre,corre,newfbsize}"
  allow_input="${allow_input:-0}"
  echo "==> ${label}: ${host}:${port} (${encodings})"
  if [[ "$allow_input" == "1" ]]; then
    UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT=1 \
    UVNC_VIEWER_REAL_SERVER_ENCODINGS="$encodings" \
    "$(dirname "$0")/qt-viewer-real-server-matrix-smoke.sh" \
      "$host" "$port" "$build_dir" "$install_prefix" "${password:-}"
  else
    UVNC_VIEWER_REAL_SERVER_ENCODINGS="$encodings" \
    "$(dirname "$0")/qt-viewer-real-server-matrix-smoke.sh" \
      "$host" "$port" "$build_dir" "$install_prefix" "${password:-}"
  fi
done <"$matrix_file"
