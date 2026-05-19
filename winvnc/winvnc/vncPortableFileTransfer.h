// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_FILE_TRANSFER_H
#define UVNC_WINVNC_PORTABLE_FILE_TRANSFER_H

#include "vncPortableRfbMessages.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

enum class FileTransferMode {
    Disabled,
    RejectOnly,
    ReadOnly,
    ReadWrite,
};

struct FileTransferDecision {
    bool accepted;
    bool readPayload;
    CARD32 payloadBytes;
    CARD16 abortReason;
    std::string reason;
};

struct FileTransferDirectoryEntry {
    std::string name;
    bool directory;
    CARD32 size;
    bool inaccessible;
};

struct FileTransferRecursiveEntry {
    std::string relativePath;
    bool directory;
    CARD32 size;
    bool inaccessible;
};

struct FileTransferRecursiveSize {
    CARD32 files;
    CARD32 directories;
    CARD32 bytesLow;
    bool truncated;
};

struct FileTransferChecksumBlock {
    CARD32 offset;
    CARD32 length;
    CARD32 crc32;
};

struct FileTransferCommandResult {
    bool success;
    CARD16 responseParam;
    CARD32 status;
    std::string payload;
    std::string reason;
};

const char *FileTransferModeName(FileTransferMode mode);
bool ParseFileTransferMode(const std::string& value, FileTransferMode& mode);
CARD32 DefaultFileTransferPayloadLimit();
bool IsSafeFileTransferRelativePath(const std::string& requestedPath, std::string *reason = nullptr);
bool ResolveFileTransferPath(const std::string& root, const std::string& requestedPath, std::string& resolvedPath, std::string *reason = nullptr);
bool FileTransferMessageMayCarryPath(CARD8 contentType);
FileTransferDecision EvaluateFileTransferMessage(const FileTransferMessage& message,
                                                 FileTransferMode mode,
                                                 CARD32 payloadLimitBytes = DefaultFileTransferPayloadLimit());
bool ListFileTransferDirectory(const std::string& root,
                               const std::string& requestedPath,
                               std::vector<FileTransferDirectoryEntry>& entries,
                               std::string *reason = nullptr);
bool ListFileTransferDirectoryRecursive(const std::string& root,
                                        const std::string& requestedPath,
                                        unsigned int maxDepth,
                                        unsigned int maxEntries,
                                        std::vector<FileTransferRecursiveEntry>& entries,
                                        std::string *reason = nullptr);
bool MeasureFileTransferDirectoryRecursive(const std::string& root,
                                           const std::string& requestedPath,
                                           unsigned int maxDepth,
                                           unsigned int maxEntries,
                                           FileTransferRecursiveSize& size,
                                           std::string *reason = nullptr);
bool PrepareFileTransferUpload(const std::string& root,
                               const std::string& requestedPath,
                               std::string& finalPath,
                               std::string& temporaryPath,
                               std::string *reason = nullptr);
bool CommitFileTransferUpload(const std::string& temporaryPath,
                              const std::string& finalPath,
                              std::string *reason = nullptr);
bool ComputeFileTransferChecksums(const std::string& root,
                                  const std::string& requestedPath,
                                  CARD32 blockSize,
                                  CARD32 maxBlocks,
                                  std::vector<FileTransferChecksumBlock>& blocks,
                                  std::string *reason = nullptr);
void AbortFileTransferUpload(const std::string& temporaryPath);
FileTransferCommandResult ExecuteFileTransferCommand(const std::string& root,
                                                     CARD16 command,
                                                     const std::string& payload,
                                                     FileTransferMode mode);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_FILE_TRANSFER_H
