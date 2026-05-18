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

option(ULTRAVNC_BUILD_LINUX_CAPTURE_X11 "Build native Linux X11 capture backend when X11 development files are available" ON)
option(ULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE "Build native Linux PipeWire/XDG portal capture backend when development files are available" ON)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
if(ULTRAVNC_BUILD_LINUX_CAPTURE_X11)
    find_package(X11 QUIET)
endif()
if(ULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(WINVNC_PIPEWIRE QUIET libpipewire-0.3)
        pkg_check_modules(WINVNC_DBUS QUIET dbus-1)
    endif()
endif()
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
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRuntime.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableShutdown.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableFramebuffer.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableFramebufferPattern.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableDirtyTracker.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableDesktopSource.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableFramebufferDiff.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableCapturePipeline.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableUpdateEncoder.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableTcp.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableServerConfig.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRfb.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRfbMessages.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRfbClientState.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRfbUpdate.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableRfbSession.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncPortableMemoryServer.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxFramebufferSource.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxX11FramebufferSource.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxPipeWirePortalCapture.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxCaptureBackend.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxInputBackend.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncLinuxXTestInput.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/translate.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencoder.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencoderre.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencodehext.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencodecorre.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncencodezrle.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeZlib.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeZlibHex.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeUltra.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeUltra2.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeXZ.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/vncEncodeTight.cpp"
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
        _XZ
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ZLIB::ZLIB
        ${ULTRAVNC_WINVNC_ZSTD_TARGET}
        libminilzo
        librdr
        JPEG::JPEG
)

if(ULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE AND WINVNC_PIPEWIRE_FOUND AND WINVNC_DBUS_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UVNC_HAVE_PIPEWIRE_PORTAL=1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${WINVNC_PIPEWIRE_INCLUDE_DIRS} ${WINVNC_DBUS_INCLUDE_DIRS})
endif()

if(ULTRAVNC_BUILD_LINUX_CAPTURE_X11 AND X11_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UVNC_HAVE_X11=1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${X11_LIBRARIES})
    if(X11_Xext_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE UVNC_HAVE_X11_XSHM=1)
        target_include_directories(${PROJECT_NAME} PRIVATE ${X11_Xext_INCLUDE_PATH})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${X11_Xext_LIB})
    endif()
endif()
