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

    add_test(NAME winvnc_memory_server_validate COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --width 320 --height 240 --name smoke)
    set_tests_properties(winvnc_memory_server_validate PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_validate_pattern COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --pattern checker --width 320 --height 240 --name smoke)
    set_tests_properties(winvnc_memory_server_validate_pattern PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_print_config COMMAND uvnc_winvnc_memory_server --allow-no-auth --print-config --pattern gradient-y --fill-byte 42 --width 64 --height 32 --name printed-config --max-updates 5)
    set_tests_properties(winvnc_memory_server_print_config PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_config_file COMMAND "${CMAKE_SOURCE_DIR}/tests/winvnc_memory_server_config_file_smoke.sh" $<TARGET_FILE:uvnc_winvnc_memory_server>)
    set_tests_properties(winvnc_memory_server_config_file PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-server-integration" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_security_config COMMAND "${CMAKE_SOURCE_DIR}/tests/winvnc_memory_server_security_config_smoke.sh" $<TARGET_FILE:uvnc_winvnc_memory_server>)
    set_tests_properties(winvnc_memory_server_security_config PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-server-security" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_invalid_width COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --width 0)
    set_tests_properties(winvnc_memory_server_invalid_width PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_invalid_fill_byte COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --fill-byte 256)
    set_tests_properties(winvnc_memory_server_invalid_fill_byte PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_invalid_bind_address COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --bind-address localhost)
    set_tests_properties(winvnc_memory_server_invalid_bind_address PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_invalid_pattern COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --pattern unknown)
    set_tests_properties(winvnc_memory_server_invalid_pattern PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_smoke COMMAND uvnc_winvnc_memory_server --smoke-test --width 64 --height 32 --name memory-smoke)
    set_tests_properties(winvnc_memory_server_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_update_smoke COMMAND uvnc_winvnc_memory_server --smoke-update-test --fill-byte 85 --width 64 --height 32 --name memory-update-smoke)
    set_tests_properties(winvnc_memory_server_update_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_multi_update_smoke COMMAND uvnc_winvnc_memory_server --smoke-multi-update-test --max-updates 3 --fill-byte 119 --width 64 --height 32 --name memory-multi-update-smoke)
    set_tests_properties(winvnc_memory_server_multi_update_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_raw_file_update_smoke COMMAND uvnc_winvnc_memory_server --smoke-raw-file-update-test --fill-byte 153 --width 64 --height 32 --name raw-file-update-smoke)
    set_tests_properties(winvnc_memory_server_raw_file_update_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-server-real" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_invalid_max_updates COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --max-updates 0)
    set_tests_properties(winvnc_memory_server_invalid_max_updates PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_print_capture_auto COMMAND uvnc_winvnc_memory_server --allow-no-auth --print-config --capture-backend auto)
    set_tests_properties(winvnc_memory_server_print_capture_auto PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 PASS_REGULAR_EXPRESSION "resolved_capture_backend=(memory|x11)")

    add_test(NAME winvnc_memory_server_invalid_capture_backend COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --capture-backend unknown)
    set_tests_properties(winvnc_memory_server_invalid_capture_backend PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_pipewire_availability_smoke COMMAND uvnc_winvnc_memory_server --smoke-pipewire-availability-test)
    set_tests_properties(winvnc_memory_server_pipewire_availability_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_x11_capture_backend_cli_smoke COMMAND "${CMAKE_SOURCE_DIR}/tests/winvnc_x11_capture_backend_cli_smoke.sh" $<TARGET_FILE:uvnc_winvnc_memory_server>)
    set_tests_properties(winvnc_memory_server_x11_capture_backend_cli_smoke PROPERTIES LABELS "native-linux;winvnc-memory-server" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_x11_availability COMMAND uvnc_winvnc_memory_server --smoke-x11-availability-test)
    set_tests_properties(winvnc_memory_server_x11_availability PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-server-real" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_xtest_availability COMMAND uvnc_winvnc_memory_server --smoke-xtest-availability-test)
    set_tests_properties(winvnc_memory_server_xtest_availability PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-input" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_invalid_input_backend COMMAND uvnc_winvnc_memory_server --allow-no-auth --validate-config --input-backend invalid)
    set_tests_properties(winvnc_memory_server_invalid_input_backend PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-input" TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME winvnc_memory_server_print_input_backend COMMAND uvnc_winvnc_memory_server --allow-no-auth --print-config --input-backend none)
    set_tests_properties(winvnc_memory_server_print_input_backend PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-input" TIMEOUT 5)

    add_test(NAME winvnc_memory_server_xtest_input_guarded COMMAND uvnc_winvnc_memory_server --smoke-xtest-input-test)
    set_tests_properties(winvnc_memory_server_xtest_input_guarded PROPERTIES LABELS "native-linux;winvnc-memory-server;linux-input" TIMEOUT 5)
endif()

install(TARGETS uvnc_winvnc_memory_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
