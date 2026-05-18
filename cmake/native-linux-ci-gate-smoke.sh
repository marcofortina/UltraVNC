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
BUILD_ROOT="${1:-/tmp/uvnc-native-linux-ci-gate-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-native-linux-ci-gate-install}"

mkdir -p "${BUILD_ROOT}"

# This gate is intentionally CI-safe: live desktop, live input injection and
# live systemd user-service checks remain opt-in manual validations.
UVNC_RUN_REAL_X11_SERVER=0 \
UVNC_RUN_SYSTEMD_USER_SERVICE=0 \
UVNC_RUN_XTEST_LIVE=0 \
  "${SCRIPT_DIR}/native-linux-closure-smoke.sh" \
    "${BUILD_ROOT}/repeater" \
    "${BUILD_ROOT}/winvnc" \
    "${INSTALL_PREFIX}" \
    "${BUILD_ROOT}/qt"


UVNC_RUN_REAL_X11_SERVER=0 \
  "${SCRIPT_DIR}/linux-server-real-runtime-smoke.sh" \
    "${BUILD_ROOT}/linux-server-real-runtime" \
    "${INSTALL_PREFIX}"

"${SCRIPT_DIR}/linux-server-negative-runtime-smoke.sh" \
  "${BUILD_ROOT}/linux-server-negative-runtime" \
  "${INSTALL_PREFIX}"

UVNC_RUN_SYSTEMD_USER_SERVICE=0 \
  "${SCRIPT_DIR}/linux-server-user-service-smoke.sh" \
    "${BUILD_ROOT}/linux-server-user-service" \
    "${INSTALL_PREFIX}"

cat <<EOF_SUMMARY
Native Linux CI gate smoke passed.
Live checks intentionally deferred:
- UVNC_RUN_REAL_X11_SERVER=1 cmake/linux-server-real-runtime-smoke.sh ...
- UVNC_RUN_SYSTEMD_USER_SERVICE=1 cmake/linux-server-user-service-smoke.sh ...
- server compatibility matrix with external VNC viewers
EOF_SUMMARY
