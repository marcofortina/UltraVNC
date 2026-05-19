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
    const std::string root = std::string("/tmp/uvnc-ft-overwrite-") + std::to_string(getpid());
    assert(mkdir(root.c_str(), 0700) == 0);
    {
        std::ofstream out((root + "/existing.txt").c_str(), std::ios::binary | std::ios::trunc);
        out << "old";
    }

    std::string finalPath;
    std::string temporaryPath;
    std::string reason;
    assert(!PrepareFileTransferUpload(root, "existing.txt", false, finalPath, temporaryPath, &reason));
    assert(reason.find("overwrite is disabled") != std::string::npos);
    assert(PrepareFileTransferUpload(root, "existing.txt", true, finalPath, temporaryPath, &reason));
    AbortFileTransferUpload(temporaryPath);

    unlink((root + "/existing.txt").c_str());
    rmdir(root.c_str());
    return 0;
}
