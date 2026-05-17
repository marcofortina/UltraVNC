# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(librdr LANGUAGES CXX)

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG QUIET)
if(TARGET zstd::libzstd)
    set(ULTRAVNC_ZSTD_TARGET zstd::libzstd)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
    set(ULTRAVNC_ZSTD_TARGET PkgConfig::ZSTD)
endif()
find_package(LibLZMA REQUIRED)

set(${PROJECT_NAME}_SOURCES
    FdInStream.cxx
    FdOutStream.cxx
    InStream.cxx
    NullOutStream.cxx
    ZlibInStream.cxx
    ZlibOutStream.cxx
    ZstdInStream.cxx
    ZstdOutStream.cxx
    xzInStream.cxx
    xzOutStream.cxx
)
add_library(${PROJECT_NAME} STATIC)
target_compile_definitions(${PROJECT_NAME} PRIVATE _XZ _VCPKG)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../rdr/${file}")
endforeach(file)
if(NOT WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE -include "${CMAKE_SOURCE_DIR}/native_compat.h")
endif()
target_link_libraries(${PROJECT_NAME}
    PRIVATE ZLIB::ZLIB
    PRIVATE ${ULTRAVNC_ZSTD_TARGET}
    PRIVATE LibLZMA::LibLZMA
)
