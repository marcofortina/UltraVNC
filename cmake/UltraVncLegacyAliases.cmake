# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

function(uvnc_add_legacy_executable_alias target legacy_name)
    if(WIN32)
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Cannot add legacy alias ${legacy_name}: target ${target} does not exist")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE_DIR:${target}>/${legacy_name}"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "$<TARGET_FILE_NAME:${target}>" "$<TARGET_FILE_DIR:${target}>/${legacy_name}"
        VERBATIM
    )

    set(_uvnc_legacy_install_dir "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
    set(_uvnc_legacy_install_link "${_uvnc_legacy_install_dir}/${legacy_name}")
    install(CODE "
file(MAKE_DIRECTORY \"${_uvnc_legacy_install_dir}\")
file(REMOVE \"${_uvnc_legacy_install_link}\")
execute_process(
    COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"$<TARGET_FILE_NAME:${target}>\" \"${_uvnc_legacy_install_link}\"
    RESULT_VARIABLE _uvnc_legacy_alias_result
)
if(NOT _uvnc_legacy_alias_result EQUAL 0)
    message(FATAL_ERROR \"failed to create installed legacy alias: ${legacy_name}\")
endif()
")
endfunction()
