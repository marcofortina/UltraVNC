#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

if [[ "$#" -lt 1 ]]; then
  echo "usage: $0 <matrix-file> [required-labels-comma-separated]" >&2
  exit 2
fi

matrix_file="$1"
required_labels="${2:-${UVNC_VIEWER_REQUIRED_MATRIX_LABELS:-}}"

if [[ ! -r "$matrix_file" ]]; then
  echo "matrix file is not readable: $matrix_file" >&2
  exit 2
fi

seen_labels=()
line_no=0
while IFS= read -r line || [[ -n "$line" ]]; do
  line_no=$((line_no + 1))
  [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
  IFS='|' read -r label host port password encodings allow_input extra <<<"$line"
  if [[ -n "${extra:-}" || -z "${label:-}" || -z "${host:-}" || -z "${port:-}" ]]; then
    echo "invalid matrix line ${line_no}: ${line}" >&2
    exit 2
  fi
  seen_labels+=("$label")
done <"$matrix_file"

if [[ "${#seen_labels[@]}" -eq 0 ]]; then
  echo "matrix file has no active entries: $matrix_file" >&2
  exit 2
fi

if [[ -n "$required_labels" ]]; then
  IFS=',' read -r -a required <<<"$required_labels"
  for required_label in "${required[@]}"; do
    required_label="${required_label//[[:space:]]/}"
    [[ -z "$required_label" ]] && continue
    found=0
    for label in "${seen_labels[@]}"; do
      if [[ "$label" == "$required_label" ]]; then
        found=1
        break
      fi
    done
    if [[ "$found" -ne 1 ]]; then
      echo "required matrix label is missing: $required_label" >&2
      exit 1
    fi
  done
fi

printf 'matrix-labels='
(IFS=','; echo "${seen_labels[*]}")
