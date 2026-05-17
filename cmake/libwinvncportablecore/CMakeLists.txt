# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //

project(libwinvncportablecore LANGUAGES CXX)

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG QUIET)
if(TARGET zstd::libzstd)
    set(ULTRAVNC_WINVNC_ZSTD_TARGET zstd::libzstd)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WINVNC_ZSTD REQUIRED IMPORTED_TARGET libzstd)
    set(ULTRAVNC_WINVNC_ZSTD_TARGET PkgConfig::WINVNC_ZSTD)
endif()

add_library(${PROJECT_NAME} STATIC
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/rfbRegion_portable.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/rfbUpdateTracker.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/stdhdrs_portable.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/translate.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencoder.cpp"
    "${CMAKE_SOURCE_DIR}/../common/UltraVncZ.cpp"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_SOURCE_DIR}/.."
        "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        UVNC_USE_PORTABLE_REGIONS=1
        _VCPKG
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ZLIB::ZLIB
        ${ULTRAVNC_WINVNC_ZSTD_TARGET}
)
