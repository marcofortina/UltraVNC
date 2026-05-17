#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

server="$1"

if [[ -z "${DISPLAY:-}" ]]; then
    if "${server}" --validate-config --capture-backend x11; then
        echo "X11 backend unexpectedly validated without DISPLAY" >&2
        exit 1
    fi
    exit 0
fi

"${server}" --validate-config --capture-backend x11
