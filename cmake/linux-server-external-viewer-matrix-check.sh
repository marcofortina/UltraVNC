#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <matrix-file> [required-labels-comma-separated]" >&2
  echo "matrix columns: label|viewer|status|command|notes" >&2
  exit 2
fi

matrix_file="$1"
required_labels="${2:-tigervnc,libvnc,realvnc,ultravnc-windows}"

if [[ ! -r "${matrix_file}" ]]; then
  echo "matrix file is not readable: ${matrix_file}" >&2
  exit 1
fi

declare -A labels=()
active_rows=0
line_no=0
while IFS= read -r line || [[ -n "${line}" ]]; do
  line_no=$((line_no + 1))
  [[ -z "${line}" || "${line}" =~ ^[[:space:]]*# ]] && continue

  IFS='|' read -r label viewer status command notes extra <<<"${line}"
  if [[ -n "${extra:-}" || -z "${label:-}" || -z "${viewer:-}" || -z "${status:-}" || -z "${command:-}" ]]; then
    echo "invalid matrix line ${line_no}: ${line}" >&2
    exit 1
  fi
  case "${status}" in
    ready|manual|missing) ;;
    *)
      echo "invalid matrix status at line ${line_no}: ${status}" >&2
      exit 1
      ;;
  esac
  if [[ "${status}" == "ready" && "${command}" == manual:* ]]; then
    echo "ready matrix line ${line_no} must have an executable command, not ${command}" >&2
    exit 1
  fi
  if [[ "${status}" != "ready" && "${command}" != manual:* ]]; then
    echo "non-ready matrix line ${line_no} must use manual:<reason> command" >&2
    exit 1
  fi
  labels["${label}"]="${status}"
  active_rows=$((active_rows + 1))
done <"${matrix_file}"

if [[ ${active_rows} -eq 0 ]]; then
  echo "matrix file has no active entries: ${matrix_file}" >&2
  exit 1
fi

IFS=',' read -ra required <<<"${required_labels}"
missing=0
for required_label in "${required[@]}"; do
  [[ -z "${required_label}" ]] && continue
  if [[ -z "${labels[${required_label}]:-}" ]]; then
    echo "required external viewer matrix label is missing: ${required_label}" >&2
    missing=1
  fi
done

if [[ ${missing} -ne 0 ]]; then
  exit 1
fi

printf 'external-viewer-matrix-labels='
first=1
for label in "${!labels[@]}"; do
  if [[ ${first} -eq 0 ]]; then printf ','; fi
  first=0
  printf '%s:%s' "${label}" "${labels[${label}]}"
done
printf '\n'
