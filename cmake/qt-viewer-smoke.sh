#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

build_dir="${1:-/tmp/uvnc-qt-viewer-build}"
install_prefix="${2:-/tmp/uvnc-qt-viewer-install}"

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=ON

if ! cmake --build "$build_dir" --target help | grep -q '^uvnc_qt_viewer:'; then
  echo "Skipping Qt viewer smoke because Qt6 Widgets is not available." >&2
  exit 0
fi

cmake --build "$build_dir" --target uvnc_qt_viewer vncviewer_qt_surface_smoke vncviewer_qt_connection_panel_smoke -j"$(nproc)"
ctest --test-dir "$build_dir" --output-on-failure -R '^vncviewer_qt_(smoke|surface_smoke|connection_panel_smoke)$'
cmake --install "$build_dir" --prefix "$install_prefix"

QT_QPA_PLATFORM=offscreen "$install_prefix/bin/uvnc_qt_viewer" --validate-config --host 127.0.0.1 --port 5900 --view-only
QT_QPA_PLATFORM=offscreen "$install_prefix/bin/uvnc_qt_viewer" --smoke-test --host 127.0.0.1 --port 5900 --view-only
