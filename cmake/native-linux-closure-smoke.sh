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
REPEATER_BUILD_DIR="${1:-/tmp/uvnc-repeater-linux-build}"
WINVNC_BUILD_DIR="${2:-/tmp/uvnc-winvnc-portable-core-build}"
INSTALL_PREFIX="${3:-/tmp/uvnc-linux-closure-install}"

"${SCRIPT_DIR}/repeater-linux-smoke.sh" "${REPEATER_BUILD_DIR}" "${INSTALL_PREFIX}"
"${SCRIPT_DIR}/winvnc-portable-core-closure.sh" "${WINVNC_BUILD_DIR}" "${INSTALL_PREFIX}"
