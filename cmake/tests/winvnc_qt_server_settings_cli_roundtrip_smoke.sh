#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "usage: $0 /path/to/uvnc_qt_server_settings" >&2
  exit 2
fi

settings_bin="$1"
work_dir="$(mktemp -d /tmp/uvnc-qt-settings-roundtrip.XXXXXX)"
trap 'rm -rf "${work_dir}"' EXIT

config_file="${work_dir}/server.conf"
cat >"${config_file}" <<'CONF'
bind_address=0.0.0.0
port=5998
width=1024
height=768
desktop_name=Roundtrip UltraVNC Server
auth=mslogon-ii
auth_helper=/usr/local/libexec/uvnc-auth
dsm_provider=/usr/lib/ultravnc/SecureVNCPlugin.dsm
transport_security=vencrypt-x509-vnc
tls_certificate_file=/etc/ultravnc/server.crt
tls_private_key_file=/etc/ultravnc/server.key
capture_backend=x11
input_backend=xtest
clipboard_backend=x11
file_transfer_mode=read-write
file_transfer_root=/tmp
file_transfer_allow_overwrite=true
extended_clipboard=false
CONF

output="${work_dir}/roundtrip.out"
QT_QPA_PLATFORM=offscreen "${settings_bin}" --load-config-print "${config_file}" >"${output}"
grep -q '^bind_address=0.0.0.0$' "${output}"
grep -q '^auth=mslogon-ii$' "${output}"
grep -q '^auth_helper=/usr/local/libexec/uvnc-auth$' "${output}"
grep -q '^dsm_provider=/usr/lib/ultravnc/SecureVNCPlugin.dsm$' "${output}"
grep -q '^transport_security=vencrypt-x509-vnc$' "${output}"
grep -q '^capture_backend=x11$' "${output}"
grep -q '^input_backend=xtest$' "${output}"
grep -q '^clipboard_backend=x11$' "${output}"
grep -q '^file_transfer_mode=read-write$' "${output}"

QT_QPA_PLATFORM=offscreen "${settings_bin}" --print-runtime-command | grep -q '^uvnc_winvnc_memory_server --config /tmp/uvnc-winvnc-linux-server.conf --serve-forever$'

QT_QPA_PLATFORM=offscreen "${settings_bin}" --print-legacy-executable-map >"${work_dir}/legacy-map.out"
grep -q "^vncviewer=uvnc_qt_viewer$" "${work_dir}/legacy-map.out"
grep -q "^uvnc_settings=uvnc_qt_server_settings$" "${work_dir}/legacy-map.out"
grep -q "^createpassword=uvnc_winvnc_password_file$" "${work_dir}/legacy-map.out"
