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
BUILD_DIR="${1:-/tmp/uvnc-pipewire-capture-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-pipewire-capture-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"

ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'winvnc_linux_pipewire|winvnc_memory_server_pipewire'

cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-pipewire-availability-test

if "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --validate-config --capture-backend pipewire; then
  echo "PipeWire/XDG portal backend is unexpectedly serveable; live capture implementation should add a dedicated smoke before enabling this path."
else
  echo "Skipping live PipeWire/XDG portal capture smoke because this block only provides detection/scaffolding; live frame import is not implemented yet."
fi
