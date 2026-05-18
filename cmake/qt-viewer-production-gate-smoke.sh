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
  exit 2
fi

matrix_file="$1"
build_dir="${2:-/tmp/uvnc-qt-viewer-production-gate-build}"
install_prefix="${3:-/tmp/uvnc-qt-viewer-production-gate-install}"

"$(dirname "$0")/qt-viewer-compressed-encoding-smoke.sh" /tmp/uvnc-qt-viewer-production-compressed-build
"$(dirname "$0")/qt-viewer-multi-server-matrix-smoke.sh" "$matrix_file" "$build_dir" "$install_prefix"
