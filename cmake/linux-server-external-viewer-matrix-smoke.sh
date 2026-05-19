#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "usage: $0 <matrix-file> <server-host> <server-port> <password-file> [required-labels]" >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
matrix_file="$1"
server_host="$2"
server_port="$3"
password_file="$4"
required_labels="${5:-tigervnc,libvnc,realvnc,ultravnc-windows}"

"${SCRIPT_DIR}/linux-server-external-viewer-matrix-check.sh" "${matrix_file}" "${required_labels}"

if [[ ! -r "${password_file}" ]]; then
  echo "password file is not readable: ${password_file}" >&2
  exit 1
fi

manual_rows=0
ready_rows=0
line_no=0
while IFS= read -r line || [[ -n "${line}" ]]; do
  line_no=$((line_no + 1))
  [[ -z "${line}" || "${line}" =~ ^[[:space:]]*# ]] && continue

  IFS='|' read -r label viewer status command notes <<<"${line}"
  case "${status}" in
    ready)
      ready_rows=$((ready_rows + 1))
      expanded="${command//\{host\}/${server_host}}"
      expanded="${expanded//\{port\}/${server_port}}"
      expanded="${expanded//\{password_file\}/${password_file}}"
      echo "==> ${label} (${viewer})"
      bash -lc "${expanded}"
      ;;
    manual|missing)
      manual_rows=$((manual_rows + 1))
      echo "deferred external viewer matrix row: ${label} status=${status} ${notes:-}" >&2
      ;;
  esac
done <"${matrix_file}"

if [[ ${ready_rows} -eq 0 ]]; then
  echo "external viewer matrix has no executable ready rows" >&2
  exit 1
fi
if [[ ${manual_rows} -ne 0 && "${UVNC_ALLOW_INCOMPLETE_EXTERNAL_VIEWER_MATRIX:-0}" != "1" ]]; then
  echo "external viewer matrix has ${manual_rows} non-ready rows; set UVNC_ALLOW_INCOMPLETE_EXTERNAL_VIEWER_MATRIX=1 only for planning" >&2
  exit 1
fi

echo "external viewer matrix smoke passed: ready=${ready_rows} deferred=${manual_rows}"
