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

if(BUILD_TESTING)
    add_test(NAME uvnc_repeater_headless_help COMMAND ${PROJECT_NAME} --help)
endif()
