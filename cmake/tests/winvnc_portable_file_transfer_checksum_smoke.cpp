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
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace uvnc::winvnc::portable;

int main()
{
    const std::string root = std::string("/tmp/uvnc-ft-checksum-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/file.bin").c_str(), std::ios::binary | std::ios::trunc);
        out << "abcdef";
    }

    std::vector<FileTransferChecksumBlock> blocks;
    std::string reason;
    assert(ComputeFileTransferChecksums(root, "file.bin", 3, 8, blocks, &reason));
    assert(blocks.size() == 2);
    assert(blocks[0].offset == 0);
    assert(blocks[0].length == 3);
    assert(blocks[1].offset == 3);
    assert(blocks[1].length == 3);
    assert(blocks[0].crc32 != 0);
    assert(blocks[1].crc32 != 0);

    unlink((root + "/file.bin").c_str());
    rmdir(root.c_str());
    return 0;
}
