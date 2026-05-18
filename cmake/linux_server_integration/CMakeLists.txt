# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

include(GNUInstallDirs)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/uvnc-winvnc-memory-server.service"
    "${CMAKE_CURRENT_SOURCE_DIR}/uvnc-winvnc-memory-server.env.example"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/ultravnc/linux"
)
