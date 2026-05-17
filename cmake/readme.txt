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
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-update-test --pattern checker --width 64 --height 32 --name memory-update-smoke
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --smoke-multi-update-test --max-updates 3 --pattern gradient-x --width 64 --height 32 --name memory-multi-update-smoke

# Run the memory server manually for one client. --serve-updates keeps the client
# session open until the configured number of framebuffer updates has been sent.
obj-linux/winvnc_memory_server/uvnc_winvnc_memory_server --bind-address 127.0.0.1 --port 5901 --serve-updates --max-updates 3 --pattern checker --fill-byte 85 --width 64 --height 32 --name memory-manual-smoke

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

# Native Linux porting status and strategy notes:
# - docs/native-linux-porting-status.md
# - docs/native-linux-viewer-strategy.md
# - docs/native-linux-server-strategy.md

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
