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
BUILD_DIR="${1:-/tmp/uvnc-x11-capture-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-x11-capture-install}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_CAPTURE_X11=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"

ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'winvnc_linux_capture_backend|winvnc_linux_x11'

cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --validate-config --capture-backend memory

if [[ -z "${DISPLAY:-}" ]]; then
  if "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --validate-config --capture-backend x11; then
    echo "X11 backend unexpectedly validated without DISPLAY" >&2
    exit 1
  fi
  echo "Skipping live X11 capture smoke because DISPLAY is not set."
  exit 0
fi

if [[ -n "${XDG_SESSION_TYPE:-}" && "${XDG_SESSION_TYPE}" != "x11" ]]; then
  echo "Skipping live X11 capture smoke because XDG_SESSION_TYPE=${XDG_SESSION_TYPE} is not x11."
  exit 0
fi

case "${DISPLAY}" in
  localhost:*|127.0.0.1:*)
    echo "Skipping live X11 capture smoke because DISPLAY=${DISPLAY} looks like SSH X forwarding, not a local desktop capture target."
    exit 0
    ;;
esac

if ! "${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --validate-config --capture-backend x11; then
  echo "Skipping live X11 capture smoke because the X11 backend is not available for DISPLAY=${DISPLAY}."
  exit 0
fi

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" \
  --smoke-x11-update-test \
  --width 64 \
  --height 32 \
  --name x11-capture-smoke
