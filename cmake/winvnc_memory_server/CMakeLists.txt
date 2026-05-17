# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

project(uvnc_winvnc_memory_server LANGUAGES CXX)

include(GNUInstallDirs)

add_executable(${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncMemoryServerMain.cpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE libwinvncportablecore)

if(BUILD_TESTING)
    add_test(NAME winvnc_memory_server_help COMMAND uvnc_winvnc_memory_server --help)
    set_tests_properties(winvnc_memory_server_help PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_validate COMMAND uvnc_winvnc_memory_server --validate-config --width 320 --height 240 --name smoke)
    set_tests_properties(winvnc_memory_server_validate PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_invalid_width COMMAND uvnc_winvnc_memory_server --validate-config --width 0)
    set_tests_properties(winvnc_memory_server_invalid_width PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_invalid_fill_byte COMMAND uvnc_winvnc_memory_server --validate-config --fill-byte 256)
    set_tests_properties(winvnc_memory_server_invalid_fill_byte PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_smoke COMMAND uvnc_winvnc_memory_server --smoke-test --width 64 --height 32 --name memory-smoke)
    set_tests_properties(winvnc_memory_server_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_update_smoke COMMAND uvnc_winvnc_memory_server --smoke-update-test --width 64 --height 32 --name memory-update-smoke)
    set_tests_properties(winvnc_memory_server_update_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_multi_update_smoke COMMAND uvnc_winvnc_memory_server --smoke-multi-update-test --max-updates 3 --width 64 --height 32 --name memory-multi-update-smoke)
    set_tests_properties(winvnc_memory_server_multi_update_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)
endif()

install(TARGETS uvnc_winvnc_memory_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
