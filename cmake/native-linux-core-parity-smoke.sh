#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-/tmp/uvnc-native-linux-core-parity-build}"
INSTALL_DIR="${2:-/tmp/uvnc-native-linux-core-parity-install}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=ON \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

cmake --build "${BUILD_DIR}" --target \
  native_linux_legacy_parity_targets \
  native_linux_qt_legacy_parity_smokes \
  vncviewer_portable_cli_smoke \
  vncviewer_portable_config_smoke \
  vncviewer_portable_mslogon_helper_smoke \
  vncviewer_portable_mslogon_session_smoke \
  vncviewer_portable_mslogon_server_smoke \
  winvnc_portable_external_auth_smoke \
  winvnc_portable_server_config_smoke \
  winvnc_portable_dsm_provider_smoke \
  winvnc_portable_securevnc_provider_smoke \
  -j"$(nproc)"

ctest --test-dir "${BUILD_DIR}" --output-on-failure \
  -R 'vncviewer_qt_(legacy_alias|smoke|surface_smoke|connection_panel_smoke)|winvnc_qt_server_settings_(legacy_alias|smoke|cli_smoke|roundtrip_smoke|visual_parity_report_smoke|visual_parity_snapshot_smoke)|winvnc_memory_server_legacy_alias_smoke|uvnc_winvnc_password_file_legacy_alias_smoke|uvnc_winvnc_createpassword_legacy_alias_smoke|uvnc_repeater_headless_legacy_alias_smoke|vncviewer_portable_(cli|config|mslogon_helper|mslogon_session|mslogon_server)_smoke|winvnc_portable_(external_auth|server_config|dsm_provider|securevnc_provider)_smoke'

cmake --install "${BUILD_DIR}"

for executable in winvnc vncviewer uvnc_settings repeater setpasswd createpassword; do
  if [ ! -x "${INSTALL_DIR}/bin/${executable}" ]; then
    echo "missing installed legacy executable: ${INSTALL_DIR}/bin/${executable}" >&2
    exit 1
  fi
done

if [ ! -r "${INSTALL_DIR}/lib/ultravnc/SecureVNCPlugin.dsm" ]; then
  echo "missing installed SecureVNCPlugin.dsm" >&2
  exit 1
fi

"${INSTALL_DIR}/bin/uvnc_settings" --print-runtime-command >/tmp/uvnc-core-parity-runtime-command.out
"${INSTALL_DIR}/bin/vncviewer" --help >/tmp/uvnc-core-parity-viewer-help.out

grep -q -- '--config-file' /tmp/uvnc-core-parity-viewer-help.out
grep -q 'winvnc --config' /tmp/uvnc-core-parity-runtime-command.out

bash "${SCRIPT_DIR}/linux-server-operator-config-smoke.sh" "${BUILD_DIR}-operator" "${INSTALL_DIR}-operator"

echo "Native Linux core parity smoke passed."
