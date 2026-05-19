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
CMAKE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
work_dir="$(mktemp -d /tmp/uvnc-external-viewer-matrix.XXXXXX)"
trap 'rm -rf "${work_dir}"' EXIT

matrix="${work_dir}/matrix.txt"
password_file="${work_dir}/password.txt"
printf 'secret\n' >"${password_file}"
chmod 600 "${password_file}"

cat >"${matrix}" <<'MATRIX'
tigervnc|TigerVNC test stub|ready|test "{host}" = "127.0.0.1" && test "{port}" = "5901" && test -r "{password_file}"|stub
libvnc|LibVNC test stub|ready|test "{host}" = "127.0.0.1" && test "{port}" = "5901" && test -r "{password_file}"|stub
realvnc|RealVNC manual|manual|manual:gui-client-required|manual evidence required
ultravnc-windows|UltraVNC Windows manual|manual|manual:windows-client-required|manual evidence required
MATRIX

"${CMAKE_DIR}/linux-server-external-viewer-matrix-check.sh" "${matrix}"

if "${CMAKE_DIR}/linux-server-external-viewer-matrix-smoke.sh" "${matrix}" 127.0.0.1 5901 "${password_file}" >/tmp/uvnc-matrix-smoke.out 2>/tmp/uvnc-matrix-smoke.err; then
  echo "matrix smoke unexpectedly accepted incomplete manual rows" >&2
  exit 1
fi
UVNC_ALLOW_INCOMPLETE_EXTERNAL_VIEWER_MATRIX=1 \
  "${CMAKE_DIR}/linux-server-external-viewer-matrix-smoke.sh" "${matrix}" 127.0.0.1 5901 "${password_file}"

bad_matrix="${work_dir}/bad-matrix.txt"
printf 'tigervnc|TigerVNC|ready|manual:not-a-real-command|bad\n' >"${bad_matrix}"
if "${CMAKE_DIR}/linux-server-external-viewer-matrix-check.sh" "${bad_matrix}" tigervnc >/tmp/uvnc-bad-matrix.out 2>/tmp/uvnc-bad-matrix.err; then
  echo "bad matrix unexpectedly passed" >&2
  exit 1
fi
