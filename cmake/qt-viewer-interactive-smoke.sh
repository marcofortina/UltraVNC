#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

host="${1:-127.0.0.1}"
port="${2:-5900}"
build_dir="${3:-/tmp/uvnc-qt-viewer-interactive-build}"
install_prefix="${4:-/tmp/uvnc-qt-viewer-interactive-install}"

"$(dirname "$0")/qt-viewer-smoke.sh" "$build_dir" "$install_prefix"

if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
  echo "No graphical session detected; skipping visible interactive Qt viewer launch." >&2
  exit 0
fi

"$install_prefix/bin/uvnc_qt_viewer" \
  --host "$host" \
  --port "$port" \
  --view-only \
  --continuous-updates \
  --update-interval-ms 1000
