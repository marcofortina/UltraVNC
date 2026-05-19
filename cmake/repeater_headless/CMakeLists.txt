# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //

project(uvnc_repeater_headless LANGUAGES CXX)
include(GNUInstallDirs)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/../repeater/repeater.cpp"
        "${CMAKE_SOURCE_DIR}/../repeater/repeater_headless.cpp"
)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/../repeater"
        "${CMAKE_SOURCE_DIR}/../repeater/webgui"
        "${CMAKE_SOURCE_DIR}/repeater_linux_compat"
)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        librepeatercore
        Threads::Threads
)
uvnc_add_legacy_executable_alias(${PROJECT_NAME} repeater)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    set(UVNC_REPEATER_HEADLESS_VALID_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/uvnc-repeater-headless-valid.conf")
    set(UVNC_REPEATER_HEADLESS_BAD_KEY_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/uvnc-repeater-headless-bad-key.conf")
    set(UVNC_REPEATER_HEADLESS_BAD_BOOL_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/uvnc-repeater-headless-bad-bool.conf")
    file(WRITE "${UVNC_REPEATER_HEADLESS_VALID_CONFIG}" "mode1=true\nmode2=false\nviewer-port=5901\nserver-port=5500\nbind-address=127.0.0.1\nlog-dir=${CMAKE_CURRENT_BINARY_DIR}\nquiet=true\n")
    file(WRITE "${UVNC_REPEATER_HEADLESS_BAD_KEY_CONFIG}" "unknown-option=true\n")
    file(WRITE "${UVNC_REPEATER_HEADLESS_BAD_BOOL_CONFIG}" "mode1=maybe\n")

    add_test(NAME uvnc_repeater_headless_help COMMAND ${PROJECT_NAME} --help)
    add_test(NAME uvnc_repeater_headless_rejects_bad_viewer_port COMMAND ${PROJECT_NAME} --viewer-port 0)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_viewer_port PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_missing_server_port COMMAND ${PROJECT_NAME} --server-port)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_server_port PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_bad_bind_address COMMAND ${PROJECT_NAME} --bind-address not-an-ip)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_bind_address PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_validates_mode1_only COMMAND ${PROJECT_NAME} --mode1 --no-mode2 --validate-config --quiet)
    add_test(NAME uvnc_repeater_headless_rejects_disabled_modes COMMAND ${PROJECT_NAME} --no-mode2 --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_disabled_modes PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_duplicate_mode2_ports COMMAND ${PROJECT_NAME} --viewer-port 5901 --server-port 5901 --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_duplicate_mode2_ports PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_startup_smoke COMMAND ${PROJECT_NAME} --smoke-test --quiet)
    set_tests_properties(uvnc_repeater_headless_startup_smoke PROPERTIES TIMEOUT 15)
    add_test(NAME uvnc_repeater_headless_loopback_bind_smoke COMMAND ${PROJECT_NAME} --bind-address 127.0.0.1 --smoke-test --quiet)
    set_tests_properties(uvnc_repeater_headless_loopback_bind_smoke PROPERTIES TIMEOUT 15)
    add_test(NAME uvnc_repeater_headless_rejects_missing_log_dir COMMAND ${PROJECT_NAME} --log-dir)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_log_dir PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_missing_log_dir_path COMMAND ${PROJECT_NAME} --log-dir ${CMAKE_CURRENT_BINARY_DIR}/missing-log-dir --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_log_dir_path PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_validates_config_file COMMAND ${PROJECT_NAME} --config ${UVNC_REPEATER_HEADLESS_VALID_CONFIG} --validate-config --quiet)
    add_test(NAME uvnc_repeater_headless_rejects_missing_config_file COMMAND ${PROJECT_NAME} --config ${CMAKE_CURRENT_BINARY_DIR}/missing.conf --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_config_file PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_bad_config_key COMMAND ${PROJECT_NAME} --config ${UVNC_REPEATER_HEADLESS_BAD_KEY_CONFIG} --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_config_key PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_bad_config_bool COMMAND ${PROJECT_NAME} --config ${UVNC_REPEATER_HEADLESS_BAD_BOOL_CONFIG} --validate-config --quiet)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_config_bool PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_loopback_log_dir_smoke COMMAND ${PROJECT_NAME} --bind-address 127.0.0.1 --log-dir ${CMAKE_CURRENT_BINARY_DIR} --smoke-test --quiet)
    set_tests_properties(uvnc_repeater_headless_loopback_log_dir_smoke PROPERTIES TIMEOUT 15)

    set(UVNC_REPEATER_HEADLESS_TESTS
        uvnc_repeater_headless_help
        uvnc_repeater_headless_rejects_bad_viewer_port
        uvnc_repeater_headless_rejects_missing_server_port
        uvnc_repeater_headless_rejects_bad_bind_address
        uvnc_repeater_headless_validates_mode1_only
        uvnc_repeater_headless_rejects_disabled_modes
        uvnc_repeater_headless_rejects_duplicate_mode2_ports
        uvnc_repeater_headless_startup_smoke
        uvnc_repeater_headless_loopback_bind_smoke
        uvnc_repeater_headless_rejects_missing_log_dir
        uvnc_repeater_headless_rejects_missing_log_dir_path
        uvnc_repeater_headless_validates_config_file
        uvnc_repeater_headless_rejects_missing_config_file
        uvnc_repeater_headless_rejects_bad_config_key
        uvnc_repeater_headless_rejects_bad_config_bool
        uvnc_repeater_headless_loopback_log_dir_smoke
    )
    set_tests_properties(${UVNC_REPEATER_HEADLESS_TESTS}
        PROPERTIES
            LABELS "native-linux;repeater-headless"
    )
endif()
