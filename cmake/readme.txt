// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


######################

# Debian Trixie/unstable amd64 qemu VM 2024-07-28

apt install git curl zip unzip tar build-essential cmake ninja-build mingw-w64 nasm pkg-config ccache



mkdir $HOME/source
cd    $HOME/source
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

./bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=$HOME/source/vcpkg
export PATH=$VCPKG_ROOT:$PATH

vcpkg --version



export PATH=/usr/lib/ccache:$PATH

vcpkg install zlib:x64-mingw-static
vcpkg install zstd:x64-mingw-static
vcpkg install libjpeg-turbo:x64-mingw-static
vcpkg install liblzma:x64-mingw-static
vcpkg install openssl:x64-mingw-static
vcpkg install libsodium:x64-mingw-static



cd    $HOME/source
git clone https://github.com/ultravnc/UltraVNC.git

mkdir obj && cd obj
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/toolchains/mingw.cmake \
    -DCMAKE_SYSTEM_NAME=MinGW \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    -DVCPKG_TARGET_ARCHITECTURE=x64 \
    -DVCPKG_APPLOCAL_DEPS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    ../UltraVNC/cmake
cmake --build . -j
cmake --build . --target install
#make clean

cp -a /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll .
cp -a /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll .







######################

# Experimental native Linux subset

# This does not build the Windows UltraVNC server/viewer applications.
# It builds the portable native subset currently available for Linux,
# including librdr and the headless repeater target.

apt install git build-essential cmake ninja-build pkg-config zlib1g-dev libzstd-dev liblzma-dev

cd $HOME/source
git clone https://github.com/ultravnc/UltraVNC.git

cmake -S UltraVNC/cmake -B obj-linux -G Ninja \
    -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
    -DULTRAVNC_BUILD_WINDOWS_APPS=OFF
cmake --build obj-linux -j
ctest --test-dir obj-linux --output-on-failure

# Smoke check the experimental headless repeater CLI.
obj-linux/repeater_headless/uvnc_repeater_headless --help

# Start the experimental headless repeater on local free ports,
# verify that both listeners accept loopback connections, and shut it down.
obj-linux/repeater_headless/uvnc_repeater_headless --smoke-test --quiet

# Equivalent local validation helper, useful on feature branches before CI coverage
# is available on the default branch.
UltraVNC/cmake/native-linux-subset-smoke.sh obj-linux

# Validate runtime options without starting listeners.
obj-linux/repeater_headless/uvnc_repeater_headless --mode1 --no-mode2 --validate-config --quiet

# Use an explicit runtime log directory when the repeater binary is installed
# under a read-only or shared location.
mkdir -p /tmp/uvnc-repeater-logs
obj-linux/repeater_headless/uvnc_repeater_headless --bind-address 127.0.0.1 --log-dir /tmp/uvnc-repeater-logs --viewer-port 5901 --server-port 5500

# Smoke the experimental in-memory WinVNC server target. This does not capture a
# real Linux desktop yet; it verifies the native RFB handshake and raw framebuffer
# update path against an in-memory framebuffer.
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-test --width 64 --height 32 --name memory-smoke
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-update-test --width 64 --height 32 --name memory-update-smoke
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-multi-update-test --max-updates 3 --width 64 --height 32 --name memory-multi-update-smoke

# Run the memory server manually for one client. --serve-updates keeps the client
# session open until the configured number of framebuffer updates has been sent.
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --bind-address 127.0.0.1 --port 5901 --serve-updates --max-updates 3 --fill-byte 85 --width 64 --height 32 --name memory-manual-smoke

# Notes:
# - --bind-address accepts IPv4 addresses only in the current native subset.
# - --fill-byte and --pattern control the synthetic framebuffer content used
#   by the memory server while real Linux desktop capture backends are still pending.
# - incremental framebuffer requests return empty updates when the synthetic
#   framebuffer has not changed.

# Load headless options from a simple key=value config file. Command-line
# options are parsed in order, so options after --config override file values.
cat >/tmp/uvnc-repeater-headless.conf <<EOF
mode1=true
mode2=false
viewer-port=5901
server-port=5500
bind-address=127.0.0.1
log-dir=/tmp/uvnc-repeater-logs
quiet=true
EOF
obj-linux/repeater_headless/uvnc_repeater_headless --config /tmp/uvnc-repeater-headless.conf --validate-config --quiet

# The same native Linux subset also builds the WinVNC portable core slice.
# This is not a Linux server yet; it now covers protocol/update tracking,
# encoder paths, runtime hooks, framebuffer/capture seams and portable tests.
ctest --test-dir obj-linux --output-on-failure -L winvnc-portable

# Dedicated closure checks for the currently completed Linux milestones:
UltraVNC/cmake/repeater-linux-smoke.sh /tmp/uvnc-repeater-linux-build /tmp/uvnc-linux-closure-install
UltraVNC/cmake/winvnc-portable-core-closure.sh /tmp/uvnc-winvnc-portable-core-build /tmp/uvnc-linux-closure-install
UltraVNC/cmake/native-linux-closure-smoke.sh /tmp/uvnc-repeater-linux-build /tmp/uvnc-winvnc-portable-core-build /tmp/uvnc-linux-closure-install

# Native Linux input backend smoke. This validates registry/selection and XTest
# availability without injecting input. Live XTest injection is opt-in because it
# affects the active X11 session.
UltraVNC/cmake/linux-input-smoke.sh /tmp/uvnc-linux-input-build /tmp/uvnc-linux-input-install
UVNC_RUN_XTEST_LIVE=1 UltraVNC/cmake/linux-input-smoke.sh /tmp/uvnc-linux-input-build /tmp/uvnc-linux-input-install

# Native Linux porting status and strategy notes:
# - docs/native-linux-porting-status.md
# - docs/native-linux-milestone-closure.md
# - docs/native-linux-viewer-strategy.md
# - docs/native-linux-server-strategy.md
# - docs/native-linux-input-backend.md

######################

# Windows with cmake, generate Visual Studio project files

# Install git
# Install Visual Studio Community 2022 (with MFC components to avoid errors about missing afxres.h)

# Open git bash

mkdir c:/source
cd c:/source
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg


# x64 Native Tools Command Prompt for VS 2022

cd /d c:\source\vcpkg
bootstrap-vcpkg.bat -disableMetrics
set VCPKG_ROOT=c:\source\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%

vcpkg --version


# If you get this message:
#   vcpkg could not locate a manifest (vcpkg.json);
#   https://superuser.com/questions/1829880/vcpkg-could-not-locate-a-manifest-vcpkg-json
doskey vcpkg=

vcpkg install zlib:x64-windows-static
vcpkg install zstd:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows-static
vcpkg install liblzma:x64-windows-static
vcpkg install openssl:x64-windows-static
vcpkg install libsodium:x64-windows-static

vcpkg integrate install


cd /d c:\source
git clone https://github.com/ultravnc/UltraVNC.git

mkdir obj && cd obj
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    ..\UltraVNC\cmake
set CL=/MP
cmake --build . --parallel --config=RelWithDebInfo




######################

# Windows with cmake, using ninja build system, and address santitizer enabled

# x64 Native Tools Command Prompt for VS 2022

# Same steps before the cmake invocation as above

cd /d c:\source
mkdir obj_ninja && cd obj_ninja
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    -G Ninja ^
    -Dasan=TRUE ^
    ..\UltraVNC\cmake
cmake --build . --parallel --config=RelWithDebInfo
cmake --build . --target install --config=RelWithDebInfo
copy "%VCToolsInstallDir%\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" ultravnc\
#ninja clean




######################

# Windows with cmake, using LLVM compiler

# x64 Native Tools Command Prompt for VS 2022

# Same steps before the cmake invocation as above

cd /d c:\source
mkdir obj_ninja_llvm && cd obj_ninja_llvm
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    -T ClangCL ^
    ..\UltraVNC\cmake
cmake --build . --parallel --config=RelWithDebInfo
#ninja clean




######################

# Windows with regular Visual Studio project files

# Install Plattformtoolset matching to the project files, currently v142

set _P=^
  /p:Platform=x64 ^
  /p:Configuration=Release ^
  /p:Plattformtoolset=v143 ^
  /p:BuildInParallel=true -maxcpucount:16 /p:CL_MPCount=16 ^
  /t:Clean;Build
set CL=/MP

cd /d C:\source\UltraVNC
msbuild %_P% winvnc\winvnc.sln
msbuild %_P% vncviewer\vncviewer.sln

# Dedicated Memory server RFB closure smoke.
cmake/memory-server-rfb-closure.sh /tmp/uvnc-memory-server-rfb-build /tmp/uvnc-linux-closure-install

Native Linux capture backend registry:

```sh
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --print-config --capture-backend auto

# Build and smoke the optional X11 capture backend. The live X11 capture step is
# skipped when DISPLAY is not usable, so this helper is safe for headless CI.
cmake/x11-capture-smoke.sh /tmp/uvnc-x11-capture-build /tmp/uvnc-x11-capture-install

# Build and smoke the optional PipeWire/XDG portal capture skeleton. The helper
# reports runtime availability and skips live capture when no Wayland/portal
# session is available, so it is safe for headless CI.
cmake/pipewire-capture-smoke.sh /tmp/uvnc-pipewire-capture-build /tmp/uvnc-pipewire-capture-install

obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --validate-config --capture-backend memory
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-pipewire-availability-test
```

Native Linux server backend integration smoke:

```sh
cmake/linux-server-integration-smoke.sh \
  /tmp/uvnc-linux-server-integration-build \
  /tmp/uvnc-linux-server-integration-install
```

Installed integration examples are placed under:

```sh
/tmp/uvnc-linux-server-integration-install/share/ultravnc/linux/
```

Native Linux Qt viewer shell smoke:

```sh
cmake/qt-viewer-smoke.sh \
  /tmp/uvnc-qt-viewer-build \
  /tmp/uvnc-qt-viewer-install
```

The Qt viewer shell is optional and requires Qt6 Widgets development files. The
smoke uses `QT_QPA_PLATFORM=offscreen` for CI-safe event-loop, framebuffer
surface and local keyboard/pointer event validation.

Visible interactive Qt viewer launch against a known server:

```sh
cmake/qt-viewer-interactive-smoke.sh 127.0.0.1 5900   /tmp/uvnc-qt-viewer-interactive-build   /tmp/uvnc-qt-viewer-interactive-install
```

The visible helper opens the interactive Qt shell with host/port/options controls
and continuous update polling enabled.

Native Linux Qt viewer RFB session smoke:

```sh
cmake/qt-viewer-rfb-smoke.sh \
  /tmp/uvnc-qt-viewer-rfb-build \
  /tmp/uvnc-qt-viewer-rfb-install
```

This starts `uvnc_winvnc_memory_server` on loopback and verifies that
`uvnc_qt_viewer --connect-update-smoke` completes a handshake, reads one raw
framebuffer update, and renders an update into the Qt surface.

Native Linux Qt viewer persistent input smoke:

```sh
cmake/qt-viewer-persistent-input-smoke.sh \
  /tmp/uvnc-qt-viewer-persistent-input-build \
  /tmp/uvnc-qt-viewer-persistent-input-install
```

This keeps one RFB connection open, sends keyboard and pointer events, then
requests a raw framebuffer update on the same session.

Native Linux Qt viewer known-server manual smoke:

```sh
cmake/qt-viewer-known-server-smoke.sh \
  127.0.0.1 \
  5900 \
  /tmp/uvnc-qt-viewer-known-server-build \
  /tmp/uvnc-qt-viewer-known-server-install
```

The known-server helper expects an RFB 3.8 no-auth raw update test server.

Native Linux Qt viewer real-server matrix smoke:

```sh
cmake/qt-viewer-real-server-matrix-smoke.sh \
  127.0.0.1 \
  5901 \
  /tmp/uvnc-real-server-build \
  /tmp/uvnc-real-server-install \
  secret
```

Set `UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT=1` only when it is acceptable for the
smoke to send key, pointer and clipboard events to that real server.
