// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFileTransfer.h"

#include <cassert>
#include <string>

using uvnc::winvnc::portable::IsSafeFileTransferRelativePath;
using uvnc::winvnc::portable::ResolveFileTransferPath;

int main()
{
    std::string reason;
    std::string resolved;
    assert(IsSafeFileTransferRelativePath("dir/file.txt", &reason));
    assert(reason.empty());
    assert(ResolveFileTransferPath("/srv/uvnc-files", "dir/file.txt", resolved, &reason));
    assert(resolved == "/srv/uvnc-files/dir/file.txt");

    assert(!IsSafeFileTransferRelativePath("/etc/passwd", &reason));
    assert(!IsSafeFileTransferRelativePath("../secret", &reason));
    assert(!IsSafeFileTransferRelativePath("dir/../secret", &reason));
    assert(!IsSafeFileTransferRelativePath("dir//secret", &reason));
    assert(!ResolveFileTransferPath("relative-root", "file.txt", resolved, &reason));
    return 0;
}
