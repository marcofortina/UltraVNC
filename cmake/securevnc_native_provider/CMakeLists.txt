# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

project(uvnc_securevnc_provider LANGUAGES CXX)

find_package(OpenSSL QUIET)
if(NOT TARGET OpenSSL::Crypto)
    message(STATUS "OpenSSL not found; skipping native SecureVNC provider")
    return()
endif()

add_library(uvnc_securevnc_provider SHARED
    "${CMAKE_SOURCE_DIR}/../winvnc/securevnc_provider/uvncSecureVncProvider.cpp"
)

target_include_directories(uvnc_securevnc_provider PRIVATE
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
    "${CMAKE_SOURCE_DIR}/.."
)
target_link_libraries(uvnc_securevnc_provider PRIVATE OpenSSL::Crypto)
set_target_properties(uvnc_securevnc_provider PROPERTIES
    PREFIX ""
    SUFFIX ".dsm"
    OUTPUT_NAME "SecureVNCPlugin"
)

install(TARGETS uvnc_securevnc_provider
    LIBRARY DESTINATION lib/ultravnc
)
