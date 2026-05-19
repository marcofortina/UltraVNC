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
tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/uvnc-security-config.XXXXXX")"
trap 'rm -rf "${tmpdir}"' EXIT

expect_fail() {
  if "$@" >"${tmpdir}/stdout" 2>"${tmpdir}/stderr"; then
    echo "command unexpectedly passed: $*" >&2
    exit 1
  fi
}

# No-auth must be explicit even on loopback validation paths.
expect_fail "${server}" --validate-config
grep -q 'no-auth is disabled by default' "${tmpdir}/stderr"

"${server}" --allow-no-auth --validate-config

# Non-loopback no-auth requires a second, intentionally scary lab override.
expect_fail "${server}" --allow-no-auth --bind-address 0.0.0.0 --validate-config
grep -q 'refusing no-auth on a non-loopback bind address' "${tmpdir}/stderr"

"${server}" --allow-no-auth --allow-public-no-auth --bind-address 0.0.0.0 --validate-config

password_file="${tmpdir}/vnc-password"
printf 'secret1\n' >"${password_file}"
chmod 0644 "${password_file}"
expect_fail "${server}" --password-file "${password_file}" --validate-config
grep -q 'password file must not be accessible by group/other' "${tmpdir}/stderr"

chmod 0600 "${password_file}"
"${server}" --password-file "${password_file}" --validate-config
expect_fail "${server}" --password-file "${password_file}" --bind-address 0.0.0.0 --validate-config
grep -q 'refusing VNCAuth on a non-loopback bind without transport encryption' "${tmpdir}/stderr"
"${server}" --password-file "${password_file}" --bind-address 0.0.0.0 --allow-unencrypted-public --validate-config >"${tmpdir}/public.out" 2>"${tmpdir}/public.err"
grep -q 'listening on 0.0.0.0' "${tmpdir}/public.err"
grep -q 'VNCAuth protects the handshake' "${tmpdir}/public.err"
"${server}" --password-file "${password_file}" --smoke-test --width 32 --height 16 --name vncauth-cli-smoke

output="$(${server} --password-file "${password_file}" --print-config)"
grep -q '^auth=vnc-password$' <<<"${output}"
if grep -qi 'secret1' <<<"${output}"; then
  echo "resolved config leaked password" >&2
  exit 1
fi

long_password="${tmpdir}/long-password"
printf 'toolong-password\n' >"${long_password}"
chmod 0600 "${long_password}"
expect_fail "${server}" --password-file "${long_password}" --validate-config
grep -q 'VNCAuth password file value must be at most 8 bytes' "${tmpdir}/stderr"

if command -v openssl >/dev/null 2>&1; then
  tls_dir="${tmpdir}/tls"
  mkdir -p "${tls_dir}"
  cert_file="${tls_dir}/server.crt"
  key_file="${tls_dir}/server.key"
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "${key_file}" \
    -out "${cert_file}" \
    -subj '/CN=localhost' \
    -days 1 >/dev/null 2>&1
  chmod 0644 "${cert_file}"
  chmod 0600 "${key_file}"

  expect_fail "${server}" \
    --password-file "${password_file}" \
    --transport-security vencrypt-x509-vnc \
    --tls-cert-file "${cert_file}" \
    --validate-config
  grep -q 'VeNCrypt TLS mode requires tls_certificate_file and tls_private_key_file' "${tmpdir}/stderr"

  "${server}" \
    --password-file "${password_file}" \
    --bind-address 0.0.0.0 \
    --transport-security vencrypt-x509-vnc \
    --tls-cert-file "${cert_file}" \
    --tls-key-file "${key_file}" \
    --validate-config >"${tmpdir}/tls-public.out" 2>"${tmpdir}/tls-public.err"

  if grep -q 'VNCAuth protects the handshake' "${tmpdir}/tls-public.err"; then
    echo "TLS-enabled VNCAuth still printed unencrypted VNCAuth warning" >&2
    exit 1
  fi

  tls_output="$(${server} \
    --password-file "${password_file}" \
    --transport-security vencrypt-x509-vnc \
    --tls-cert-file "${cert_file}" \
    --tls-key-file "${key_file}" \
    --print-config)"
  grep -q '^transport_security=vencrypt-x509-vnc$' <<<"${tls_output}"
  grep -q '^tls_private_key_file=<configured>$' <<<"${tls_output}"
  if grep -q "${key_file}" <<<"${tls_output}"; then
    echo "resolved config leaked TLS private key path" >&2
    exit 1
  fi
fi
