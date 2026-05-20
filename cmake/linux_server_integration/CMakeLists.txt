# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

include(GNUInstallDirs)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
    "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.env.example"
    "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-linux-server.conf.example"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)


if(BUILD_TESTING)
    add_test(NAME linux_server_integration_templates_smoke
        COMMAND "${CMAKE_SOURCE_DIR}/tests/linux_server_integration_templates_smoke.sh"
                "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
                "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.env.example"
                "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-linux-server.conf.example")
    set_tests_properties(linux_server_integration_templates_smoke PROPERTIES LABELS "native-linux;linux-server-integration" TIMEOUT 5)

    add_test(NAME linux_server_external_viewer_matrix_smoke
        COMMAND "${CMAKE_SOURCE_DIR}/tests/linux_server_external_viewer_matrix_smoke.sh"
    )
    set_tests_properties(linux_server_external_viewer_matrix_smoke PROPERTIES LABELS "native-linux;linux-server-integration;external-viewer-matrix" TIMEOUT 10)
    add_test(NAME linux_server_runtime_helpers_smoke
        COMMAND "${CMAKE_SOURCE_DIR}/tests/linux_server_runtime_helpers_smoke.sh"
                "${CMAKE_SOURCE_DIR}/linux-server-real-runtime-smoke.sh"
                "${CMAKE_SOURCE_DIR}/linux-server-user-service-smoke.sh"
                "${CMAKE_SOURCE_DIR}/linux-server-negative-runtime-smoke.sh"
                "${CMAKE_SOURCE_DIR}/linux-server-secure-runtime-smoke.sh"
                "${CMAKE_SOURCE_DIR}/linux-server-operator-config-smoke.sh")
    set_tests_properties(linux_server_runtime_helpers_smoke PROPERTIES LABELS "native-linux;linux-server-integration" TIMEOUT 5)
endif()

install(FILES
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-server-backend-integration.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-input-backend.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-pipewire-capture.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-server-runtime.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-service-admin-equivalents.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-security-extensions.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-mslogon-server-auth.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-ci-gate.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-milestone-closure.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-qt-viewer.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-legacy-parity.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-issue-348-status.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)

install(FILES
    "${CMAKE_SOURCE_DIR}/../docs/examples/native-linux-server-external-viewer-matrix.example"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux/examples"
)

install(PROGRAMS
    "${CMAKE_SOURCE_DIR}/native-linux-legacy-parity-smoke.sh"
    "${CMAKE_SOURCE_DIR}/native-linux-issue-348-status-smoke.sh"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)
