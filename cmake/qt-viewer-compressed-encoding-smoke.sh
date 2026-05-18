#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

build_dir="${1:-/tmp/uvnc-qt-viewer-compressed-encoding-build}"

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=ON \
  -DULTRAVNC_BUILD_QT_VIEWER=OFF

cmake --build "$build_dir" --target \
  vncviewer_portable_compressed_encoding_smoke \
  vncviewer_portable_cli_smoke \
  -j"$(nproc)"

ctest --test-dir "$build_dir" --output-on-failure \
  -R 'vncviewer_portable_(compressed_encoding|cli)_smoke'
