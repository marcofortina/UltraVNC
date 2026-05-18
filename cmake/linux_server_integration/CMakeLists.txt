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
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)


if(BUILD_TESTING)
    add_test(NAME linux_server_integration_templates_smoke
        COMMAND "${CMAKE_SOURCE_DIR}/tests/linux_server_integration_templates_smoke.sh"
                "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
                "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.env.example")
    set_tests_properties(linux_server_integration_templates_smoke PROPERTIES LABELS "native-linux;linux-server-integration" TIMEOUT 5)
endif()

install(FILES
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-server-backend-integration.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-input-backend.md"
    "${CMAKE_SOURCE_DIR}/../docs/native-linux-pipewire-capture.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)
