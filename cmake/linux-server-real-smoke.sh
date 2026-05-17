#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${1:-/tmp/uvnc-linux-server-real-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-server-real-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF

cmake --build "${BUILD_DIR}" -j"$(nproc)"

ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'winvnc_linux_raw|winvnc_linux_memory_server_external_framebuffer|winvnc_memory_server_raw_file_update_smoke'

cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
  --smoke-raw-file-update-test \
  --fill-byte 153 \
  --width 64 \
  --height 32 \
  --name linux-server-real-raw-file-smoke
