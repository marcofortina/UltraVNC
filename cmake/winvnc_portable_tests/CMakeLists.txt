# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //

project(winvnc_portable_tests LANGUAGES CXX)

add_executable(winvnc_update_tracker_smoke
    "${CMAKE_SOURCE_DIR}/tests/winvnc_update_tracker_smoke.cpp"
)

target_link_libraries(winvnc_update_tracker_smoke
    PRIVATE
        libwinvncportablecore
)

if(BUILD_TESTING)
    add_test(NAME winvnc_update_tracker_smoke COMMAND winvnc_update_tracker_smoke)
endif()

add_executable(winvnc_translate_smoke
    "${CMAKE_SOURCE_DIR}/tests/winvnc_translate_smoke.cpp"
)

target_link_libraries(winvnc_translate_smoke PRIVATE libwinvncportablecore)

add_test(NAME winvnc_translate_smoke COMMAND winvnc_translate_smoke)

add_executable(winvnc_region_smoke
    "${CMAKE_SOURCE_DIR}/tests/winvnc_region_smoke.cpp"
)

target_link_libraries(winvnc_region_smoke PRIVATE libwinvncportablecore)

add_test(NAME winvnc_region_smoke COMMAND winvnc_region_smoke)

add_executable(winvnc_update_tracker_copyrect_smoke
    "${CMAKE_SOURCE_DIR}/tests/winvnc_update_tracker_copyrect_smoke.cpp"
)

target_link_libraries(winvnc_update_tracker_copyrect_smoke PRIVATE libwinvncportablecore)

add_test(NAME winvnc_update_tracker_copyrect_smoke COMMAND winvnc_update_tracker_copyrect_smoke)
