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
config="${TMPDIR:-/tmp}/uvnc-winvnc-server-config-file-smoke.conf"
trap 'rm -f "${config}" "${config}.err"' EXIT

cat >"${config}" <<'EOF'
# Conservative native Linux server config smoke.
bind_address=127.0.0.1
port=0
width=80
height=40
name=config-file-smoke
capture_backend=memory
input_backend=none
auth=none
allow_no_auth=true
max_updates=7
serve_updates=false
serve_forever=true
pid_file=/tmp/uvnc-config-file-smoke.pid
status_file=/tmp/uvnc-config-file-smoke.status
log_file=/tmp/uvnc-config-file-smoke.log
file_transfer_mode=read-only
file_transfer_root=/tmp
file_transfer_allow_overwrite=false
file_transfer_recursive_max_depth=9
file_transfer_recursive_max_entries=123
extended_clipboard=false
extended_clipboard_text_limit=2048
EOF
chmod 0600 "${config}"

output="$(${server} --config "${config}" --print-config --width 96)"

grep -q '^bind_address=127\.0\.0\.1$' <<<"${output}"
grep -q '^port=0$' <<<"${output}"
grep -q '^width=96$' <<<"${output}"
grep -q '^height=40$' <<<"${output}"
grep -q '^name=config-file-smoke$' <<<"${output}"
grep -q '^capture_backend=memory$' <<<"${output}"
grep -q '^input_backend=none$' <<<"${output}"
grep -q '^auth=none$' <<<"${output}"
grep -q '^allow_no_auth=yes$' <<<"${output}"
grep -q '^max_updates=7$' <<<"${output}"
grep -q '^serve_forever=yes$' <<<"${output}"
grep -q '^pid_file=/tmp/uvnc-config-file-smoke.pid$' <<<"${output}"
grep -q '^status_file=/tmp/uvnc-config-file-smoke.status$' <<<"${output}"
grep -q '^log_file=/tmp/uvnc-config-file-smoke.log$' <<<"${output}"
grep -q '^file_transfer_mode=read-only$' <<<"${output}"
grep -q '^file_transfer_root=/tmp$' <<<"${output}"
grep -q '^file_transfer_allow_overwrite=false$' <<<"${output}"
grep -q '^file_transfer_recursive_max_depth=9$' <<<"${output}"
grep -q '^file_transfer_recursive_max_entries=123$' <<<"${output}"
grep -q '^extended_clipboard=no$' <<<"${output}"
grep -q '^extended_clipboard_text_limit=2048$' <<<"${output}"

cat >"${config}" <<'EOF'
unknown_key=value
EOF
chmod 0600 "${config}"
if "${server}" --config "${config}" --validate-config >/dev/null 2>&1; then
  echo "invalid config file unexpectedly passed" >&2
  exit 1
fi


cat >"${config}" <<'EOF'
bind_address=127.0.0.1
allow_no_auth=true
EOF
chmod 0666 "${config}"
if "${server}" --config "${config}" --validate-config >/dev/null 2>"${config}.err"; then
  echo "world-writable config file unexpectedly passed" >&2
  exit 1
fi
grep -q 'file must not be group/world writable' "${config}.err"
chmod 0600 "${config}"
