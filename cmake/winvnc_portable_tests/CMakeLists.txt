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

function(uvnc_add_winvnc_portable_test name source)
    add_executable(${name}
        "${CMAKE_SOURCE_DIR}/tests/${source}"
    )

    target_link_libraries(${name} PRIVATE libwinvncportablecore)

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name}
        PROPERTIES
            LABELS "native-linux;winvnc-portable"
            TIMEOUT 10
    )
endfunction()

uvnc_add_winvnc_portable_test(winvnc_update_tracker_smoke winvnc_update_tracker_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_translate_smoke winvnc_translate_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_region_smoke winvnc_region_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_update_tracker_copyrect_smoke winvnc_update_tracker_copyrect_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_compression_smoke winvnc_compression_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_raw_smoke winvnc_encoder_raw_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_offset_smoke winvnc_encoder_offset_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_format_smoke winvnc_encoder_format_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_rre_fallback_smoke winvnc_encoder_rre_fallback_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_rre_metadata_smoke winvnc_encoder_rre_metadata_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_hextile_smoke winvnc_encoder_hextile_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_zlib_smoke winvnc_encoder_zlib_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_zlib_compressed_smoke winvnc_encoder_zlib_compressed_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_zstd_compressed_smoke winvnc_encoder_zstd_compressed_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_zlibhex_smoke winvnc_encoder_zlibhex_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_ultra_smoke winvnc_encoder_ultra_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_ultra_compressed_smoke winvnc_encoder_ultra_compressed_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_ultra2_smoke winvnc_encoder_ultra2_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_ultra2_jpeg_smoke winvnc_encoder_ultra2_jpeg_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_xz_smoke winvnc_encoder_xz_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_tight_smoke winvnc_encoder_tight_smoke.cpp)
uvnc_add_winvnc_portable_test(winvnc_encoder_tight_split_smoke winvnc_encoder_tight_split_smoke.cpp)
