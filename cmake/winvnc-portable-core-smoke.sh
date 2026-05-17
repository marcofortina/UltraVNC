#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC Projects. All Rights Reserved.

set -euo pipefail

build_dir="${1:-/tmp/uvnc-winvnc-portable-core-build}"

cmake -S cmake -B "$build_dir" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_REPEATER_HEADLESS=OFF \
  -DULTRAVNC_BUILD_WINVNC_PORTABLE_CORE=ON

cmake --build "$build_dir" -j"$(nproc)"
ctest --test-dir "$build_dir" --output-on-failure -R '^winvnc_'
