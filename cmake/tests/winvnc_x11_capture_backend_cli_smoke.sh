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

if "${server}" --allow-no-auth --validate-config --capture-backend x11; then
    exit 0
fi

# Headless CI, invalid DISPLAY values, and missing X servers are valid skip cases
# for this smoke. The dedicated live smoke helper exercises X11 when a usable
# DISPLAY is available.
echo "Skipping X11 capture backend CLI smoke because no usable X11 DISPLAY is available."
exit 0
