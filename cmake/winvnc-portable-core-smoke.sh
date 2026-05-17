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

expected_winvnc_tests=87
actual_winvnc_tests=$(ctest --test-dir "$build_dir" -N -R '^winvnc_' | sed -n 's/^Total Tests: //p')
if [[ -z "$actual_winvnc_tests" || "$actual_winvnc_tests" -lt "$expected_winvnc_tests" ]]; then
  echo "Expected at least $expected_winvnc_tests WinVNC portable tests, got ${actual_winvnc_tests:-0}" >&2
  exit 1
fi

ctest --test-dir "$build_dir" --output-on-failure -R '^winvnc_'
