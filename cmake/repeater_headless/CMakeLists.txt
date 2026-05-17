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

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    add_test(NAME uvnc_repeater_headless_help COMMAND ${PROJECT_NAME} --help)
    add_test(NAME uvnc_repeater_headless_rejects_bad_viewer_port COMMAND ${PROJECT_NAME} --viewer-port 0)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_viewer_port PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_missing_server_port COMMAND ${PROJECT_NAME} --server-port)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_server_port PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_rejects_bad_bind_address COMMAND ${PROJECT_NAME} --bind-address not-an-ip)
    set_tests_properties(uvnc_repeater_headless_rejects_bad_bind_address PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_startup_smoke COMMAND ${PROJECT_NAME} --smoke-test --quiet)
    add_test(NAME uvnc_repeater_headless_loopback_bind_smoke COMMAND ${PROJECT_NAME} --bind-address 127.0.0.1 --smoke-test --quiet)
    add_test(NAME uvnc_repeater_headless_rejects_missing_log_dir COMMAND ${PROJECT_NAME} --log-dir)
    set_tests_properties(uvnc_repeater_headless_rejects_missing_log_dir PROPERTIES WILL_FAIL TRUE)
    add_test(NAME uvnc_repeater_headless_loopback_log_dir_smoke COMMAND ${PROJECT_NAME} --bind-address 127.0.0.1 --log-dir ${CMAKE_CURRENT_BINARY_DIR} --smoke-test --quiet)
endif()
