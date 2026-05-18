#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-/tmp/uvnc-linux-input-build}"
INSTALL_PREFIX="${2:-/tmp/uvnc-linux-input-install}"

cmake -S cmake -B "${BUILD_DIR}" -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_LINUX_INPUT_XTEST=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -R 'winvnc_linux_input|winvnc_linux_xtest|winvnc_memory_server_.*input|winvnc_memory_server_xtest'
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-xtest-availability-test
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-xtest-input-test
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --validate-config --input-backend none
"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --print-config --input-backend none >/dev/null

if [[ "${UVNC_RUN_XTEST_LIVE:-0}" != "1" ]]; then
  echo "Skipping live XTest injection smoke because UVNC_RUN_XTEST_LIVE=1 is not set."
  exit 0
fi

if [[ -z "${DISPLAY:-}" ]]; then
  echo "Skipping live XTest injection smoke because DISPLAY is not set."
  exit 0
fi

if [[ "${XDG_SESSION_TYPE:-}" != "x11" ]]; then
  echo "Skipping live XTest injection smoke because XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-unset} is not x11."
  exit 0
fi

case "${DISPLAY}" in
  localhost:*|127.0.0.1:*|::1:*)
    echo "Skipping live XTest injection smoke because DISPLAY=${DISPLAY} looks like SSH X forwarding."
    exit 0
    ;;
esac

"${INSTALL_PREFIX}/bin/uvnc_winvnc_memory_server" --smoke-xtest-input-test --allow-input-injection
