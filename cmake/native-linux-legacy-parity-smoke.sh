#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <build-dir> <install-dir> [ctest-regex]" >&2
    exit 2
fi

BUILD_DIR="$1"
INSTALL_DIR="$2"
CTEST_REGEX="${3:-winvnc_memory_server_legacy_alias_smoke|uvnc_winvnc_password_file_legacy_alias_smoke|uvnc_winvnc_createpassword_legacy_alias_smoke|uvnc_repeater_headless_legacy_alias_smoke|winvnc_portable_(dsm_provider|securevnc_provider|server_config)_smoke|vncviewer_qt_(legacy_alias|smoke|surface_smoke|connection_panel_smoke)|winvnc_qt_server_settings_(legacy_alias|smoke|cli_smoke|visual_parity_report_smoke|visual_parity_snapshot_smoke)}"

cmake -S cmake -B "${BUILD_DIR}" -G Ninja \
    -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
    -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
    -DULTRAVNC_BUILD_QT_VIEWER=ON

cmake --build "${BUILD_DIR}" --target native_linux_legacy_parity_targets -j"$(nproc)"

ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "${CTEST_REGEX}"

cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"

for executable in winvnc vncviewer uvnc_settings repeater setpasswd createpassword; do
    path="${INSTALL_DIR}/bin/${executable}"
    if [[ ! -e "${path}" && ! -L "${path}" ]]; then
        echo "missing installed legacy executable: ${path}" >&2
        exit 1
    fi
    if [[ ! -x "${path}" ]]; then
        echo "installed legacy executable is not executable: ${path}" >&2
        exit 1
    fi
done

provider="${INSTALL_DIR}/lib/ultravnc/SecureVNCPlugin.dsm"
if [[ ! -f "${provider}" ]]; then
    echo "missing installed native SecureVNC provider: ${provider}" >&2
    exit 1
fi
