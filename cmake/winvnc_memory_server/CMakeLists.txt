# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

project(uvnc_winvnc_memory_server LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncMemoryServerMain.cpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE libwinvncportablecore)
