// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFileTransfer.h"

#include <cassert>
#include <fstream>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-recursive-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    assert(mkdir((root + "/dir").c_str(), 0700) == 0);
    assert(mkdir((root + "/dir/nested").c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/dir/nested/file.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "abc";
    }

    std::vector<FileTransferRecursiveEntry> entries;
    std::string reason;
    assert(ListFileTransferDirectoryRecursive(root, "", 8, 32, entries, &reason));

    std::set<std::string> paths;
    for (std::vector<FileTransferRecursiveEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        paths.insert(it->relativePath);
    }
    assert(paths.count("dir") == 1);
    assert(paths.count("dir/nested") == 1);
    assert(paths.count("dir/nested/file.txt") == 1);

    FileTransferRecursiveSize size;
    assert(MeasureFileTransferDirectoryRecursive(root, "", 8, 32, size, &reason));
    assert(size.files == 1);
    assert(size.directories == 2);
    assert(size.bytesLow == 3);
    assert(!size.truncated);

    unlink((root + "/dir/nested/file.txt").c_str());
    rmdir((root + "/dir/nested").c_str());
    rmdir((root + "/dir").c_str());
    rmdir(root.c_str());
    return 0;
}
