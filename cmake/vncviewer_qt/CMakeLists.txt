# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

project(vncviewer_qt LANGUAGES CXX)

find_package(Qt6 QUIET COMPONENTS Widgets)
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
if(NOT Qt6_FOUND)
    message(STATUS "Qt6 Widgets not found; skipping experimental native Linux Qt viewer shell")
    return()
endif()

add_executable(uvnc_qt_viewer
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerMain.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerSurface.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerConnectionPanel.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerConfig.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerCli.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerSession.cpp"
        "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerFileTransfer.cpp"
        "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerSecurity.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableMsLogon.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableVncAuth.cpp"
    "${CMAKE_SOURCE_DIR}/../rfb/vncauth.c"
    "${CMAKE_SOURCE_DIR}/../common/d3des.c"
)

target_include_directories(uvnc_qt_viewer PRIVATE
    "${CMAKE_SOURCE_DIR}/../vncviewer"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
    "${CMAKE_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/../common"
)

target_link_libraries(uvnc_qt_viewer PRIVATE Qt6::Widgets libwinvncportablecore ZLIB::ZLIB JPEG::JPEG)
uvnc_add_legacy_executable_alias(uvnc_qt_viewer vncviewer)

add_executable(vncviewer_qt_connection_panel_smoke
    "${CMAKE_SOURCE_DIR}/tests/vncviewer_qt_connection_panel_smoke.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerSurface.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerConnectionPanel.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerConfig.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerSession.cpp"
        "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerFileTransfer.cpp"
        "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableViewerSecurity.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableMsLogon.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/vncPortableVncAuth.cpp"
    "${CMAKE_SOURCE_DIR}/../rfb/vncauth.c"
    "${CMAKE_SOURCE_DIR}/../common/d3des.c"
)
target_include_directories(vncviewer_qt_connection_panel_smoke PRIVATE
    "${CMAKE_SOURCE_DIR}/../vncviewer"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
    "${CMAKE_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/../common"
)
target_link_libraries(vncviewer_qt_connection_panel_smoke PRIVATE Qt6::Widgets libwinvncportablecore ZLIB::ZLIB JPEG::JPEG)
add_test(NAME vncviewer_qt_connection_panel_smoke COMMAND vncviewer_qt_connection_panel_smoke)
set_tests_properties(vncviewer_qt_connection_panel_smoke PROPERTIES
    LABELS "native-linux;vncviewer-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

add_executable(vncviewer_qt_surface_smoke
    "${CMAKE_SOURCE_DIR}/tests/vncviewer_qt_surface_smoke.cpp"
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer/vncQtViewerSurface.cpp"
)
target_include_directories(vncviewer_qt_surface_smoke PRIVATE
    "${CMAKE_SOURCE_DIR}/../vncviewer/qtviewer"
)
target_link_libraries(vncviewer_qt_surface_smoke PRIVATE Qt6::Widgets)
add_test(NAME vncviewer_qt_surface_smoke COMMAND vncviewer_qt_surface_smoke)
set_tests_properties(vncviewer_qt_surface_smoke PROPERTIES
    LABELS "native-linux;vncviewer-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

add_test(NAME vncviewer_qt_legacy_alias_smoke COMMAND "${CMAKE_SOURCE_DIR}/tests/native_linux_legacy_executable_alias_smoke.sh" "$<TARGET_FILE_DIR:uvnc_qt_viewer>/vncviewer")
set_tests_properties(vncviewer_qt_legacy_alias_smoke PROPERTIES
    LABELS "native-linux;vncviewer-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

add_custom_target(vncviewer_qt_smoke
    DEPENDS uvnc_qt_viewer
)
add_test(NAME vncviewer_qt_smoke
    COMMAND uvnc_qt_viewer --smoke-test --host 127.0.0.1 --port 5900 --view-only
)
set_tests_properties(vncviewer_qt_smoke PROPERTIES
    LABELS "native-linux;vncviewer-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)


add_executable(uvnc_qt_server_settings
    "${CMAKE_SOURCE_DIR}/../winvnc/qtserver/vncQtServerSettingsMain.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/qtserver/vncQtServerSettingsPanel.cpp"
)
target_include_directories(uvnc_qt_server_settings PRIVATE
    "${CMAKE_SOURCE_DIR}/../winvnc/qtserver"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
    "${CMAKE_SOURCE_DIR}/../common"
)
target_link_libraries(uvnc_qt_server_settings PRIVATE Qt6::Widgets libwinvncportablecore)
uvnc_add_legacy_executable_alias(uvnc_qt_server_settings uvnc_settings)

add_executable(winvnc_qt_server_settings_smoke
    "${CMAKE_SOURCE_DIR}/tests/winvnc_qt_server_settings_smoke.cpp"
    "${CMAKE_SOURCE_DIR}/../winvnc/qtserver/vncQtServerSettingsPanel.cpp"
)
target_include_directories(winvnc_qt_server_settings_smoke PRIVATE
    "${CMAKE_SOURCE_DIR}/../winvnc/qtserver"
    "${CMAKE_SOURCE_DIR}/../winvnc/winvnc"
    "${CMAKE_SOURCE_DIR}/../common"
)
target_link_libraries(winvnc_qt_server_settings_smoke PRIVATE Qt6::Widgets libwinvncportablecore)
add_test(NAME winvnc_qt_server_settings_smoke COMMAND winvnc_qt_server_settings_smoke)
set_tests_properties(winvnc_qt_server_settings_smoke PROPERTIES
    LABELS "native-linux;winvnc-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

add_test(NAME winvnc_qt_server_settings_legacy_alias_smoke COMMAND "${CMAKE_SOURCE_DIR}/tests/native_linux_legacy_executable_alias_smoke.sh" "$<TARGET_FILE_DIR:uvnc_qt_server_settings>/uvnc_settings")
set_tests_properties(winvnc_qt_server_settings_legacy_alias_smoke PROPERTIES
    LABELS "native-linux;winvnc-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

add_custom_target(winvnc_qt_server_settings_cli_smoke
    DEPENDS uvnc_qt_server_settings
)
add_test(NAME winvnc_qt_server_settings_cli_smoke
    COMMAND uvnc_qt_server_settings --smoke-test
)
add_test(NAME winvnc_qt_server_settings_visual_parity_report_smoke
    COMMAND uvnc_qt_server_settings --print-visual-parity-report
)
set_tests_properties(winvnc_qt_server_settings_visual_parity_report_smoke PROPERTIES
    LABELS "native-linux;winvnc-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "legacy.window=WinVNC settings"
    TIMEOUT 10
)
add_test(NAME winvnc_qt_server_settings_visual_parity_snapshot_smoke
    COMMAND uvnc_qt_server_settings --save-visual-parity-snapshot "${CMAKE_CURRENT_BINARY_DIR}/winvnc-qt-server-settings-visual-parity.png"
)
set_tests_properties(winvnc_qt_server_settings_visual_parity_snapshot_smoke PROPERTIES
    LABELS "native-linux;winvnc-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)
set_tests_properties(winvnc_qt_server_settings_cli_smoke PROPERTIES
    LABELS "native-linux;winvnc-qt"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

install(TARGETS uvnc_qt_viewer uvnc_qt_server_settings RUNTIME DESTINATION bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/uvnc_qt_viewer.desktop" DESTINATION share/applications)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/uvnc_qt_server_settings.desktop" DESTINATION share/applications)
install(FILES "${CMAKE_SOURCE_DIR}/../vncviewer/res/logo.svg" DESTINATION share/icons/hicolor/scalable/apps RENAME uvnc_qt_viewer.svg)
install(FILES "${CMAKE_SOURCE_DIR}/../vncviewer/res/logo.svg" DESTINATION share/icons/hicolor/scalable/apps RENAME uvnc_qt_server_settings.svg)
install(FILES "${CMAKE_SOURCE_DIR}/../docs/native-linux-qt-viewer.md" DESTINATION share/doc/ultravnc)
install(FILES "${CMAKE_SOURCE_DIR}/../docs/native-linux-qt-viewer-production-checklist.md" DESTINATION share/doc/ultravnc)
install(FILES "${CMAKE_SOURCE_DIR}/../docs/examples/native-linux-qt-viewer-real-server-matrix.example" DESTINATION share/doc/ultravnc/examples)
install(FILES "${CMAKE_SOURCE_DIR}/../docs/native-linux-qt-server-settings.md" DESTINATION share/doc/ultravnc)
install(FILES "${CMAKE_SOURCE_DIR}/../docs/native-linux-dsm-provider-abi.md" DESTINATION share/doc/ultravnc)
