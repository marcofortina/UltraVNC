# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //

project(librepeatercore LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/../repeater/lists_functions.cpp"
        "${CMAKE_SOURCE_DIR}/../repeater/socket_functions.cpp"
        "${CMAKE_SOURCE_DIR}/../repeater/mode12_listener.cpp"
        "${CMAKE_SOURCE_DIR}/../repeater/mode2_listener_server.cpp"
)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/../repeater"
        "${CMAKE_SOURCE_DIR}/../repeater/webgui"
)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/repeater_linux_compat")
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()
