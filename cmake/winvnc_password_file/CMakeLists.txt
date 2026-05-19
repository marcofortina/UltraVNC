# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

project(winvnc_password_file LANGUAGES CXX)

add_executable(uvnc_winvnc_password_file
    "${CMAKE_SOURCE_DIR}/../winvnc/passwordfile/vncPasswordFileMain.cpp"
)
uvnc_add_legacy_executable_alias(uvnc_winvnc_password_file setpasswd)

if(BUILD_TESTING)
    add_test(NAME uvnc_winvnc_password_file_legacy_alias_smoke COMMAND "${CMAKE_SOURCE_DIR}/tests/native_linux_legacy_executable_alias_smoke.sh" "$<TARGET_FILE_DIR:uvnc_winvnc_password_file>/setpasswd")
    set_tests_properties(uvnc_winvnc_password_file_legacy_alias_smoke PROPERTIES LABELS "native-linux;winvnc-password-file" TIMEOUT 5)

    add_test(NAME uvnc_winvnc_password_file_help COMMAND uvnc_winvnc_password_file --help)
    set_tests_properties(uvnc_winvnc_password_file_help PROPERTIES LABELS "native-linux;winvnc-password-file" TIMEOUT 5)

    add_test(NAME uvnc_winvnc_password_file_rejects_long_password COMMAND uvnc_winvnc_password_file --output /tmp/uvnc-password-file-too-long --password too-long-password)
    set_tests_properties(uvnc_winvnc_password_file_rejects_long_password PROPERTIES LABELS "native-linux;winvnc-password-file" TIMEOUT 5 WILL_FAIL TRUE)
endif()

install(TARGETS uvnc_winvnc_password_file RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
