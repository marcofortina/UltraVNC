# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(librepeaterwebgui LANGUAGES C)

set(${PROJECT_NAME}_SOURCES
    webclib.c
    webfs.c
    webio.c
    webobjs.c
    websys.c
    webutils.c
    wsfdata.c
)
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/webgui/${file}")
endforeach(file)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/webgui")
if(NOT WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/../repeater_webgui_linux_compat.h"
    )
endif()
