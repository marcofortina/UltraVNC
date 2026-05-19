// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFileTransfer.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

std::string LastSystemError(const char *action)
{
    std::ostringstream out;
    out << action << " failed";
#ifndef _WIN32
    if (errno != 0) {
        out << ": " << std::strerror(errno);
    }
#endif
    return out.str();
}

std::string JoinRootPath(const std::string& root, const std::string& leaf)
{
    if (root.empty() || root[root.size() - 1] == '/') {
        return root + leaf;
    }
    return root + "/" + leaf;
}

bool SplitRenamePayload(const std::string& payload, std::string& from, std::string& to, std::string *reason)
{
    const std::string::size_type split = payload.find('*');
    if (split == std::string::npos || split == 0 || split + 1 >= payload.size() || payload.find('*', split + 1) != std::string::npos) {
        if (reason) *reason = "rename command payload must be old*new relative paths";
        return false;
    }
    from = payload.substr(0, split);
    to = payload.substr(split + 1);
    return true;
}

CARD32 CommandStatus(bool success)
{
    return success ? 0U : static_cast<CARD32>(rfbRErrorCmd);
}

FileTransferCommandResult MakeCommandResult(CARD16 response, const std::string& payload, bool success, const std::string& reason = std::string())
{
    FileTransferCommandResult result;
    result.success = success;
    result.responseParam = response;
    result.status = CommandStatus(success);
    result.payload = payload;
    result.reason = reason;
    return result;
}

bool IsRegularFilePath(const std::string& path, std::string *reason)
{
#ifndef _WIN32
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        if (reason) *reason = LastSystemError("stat");
        return false;
    }
    if (S_ISLNK(st.st_mode) || !S_ISREG(st.st_mode)) {
        if (reason) *reason = "file-transfer path is not a regular file";
        return false;
    }
    return true;
#else
    (void)path;
    if (reason) *reason = "portable file-transfer filesystem helpers are not used on Windows builds";
    return false;
#endif
}


#ifndef _WIN32
CARD32 SaturatingFileSize(off_t size)
{
    if (size <= 0) {
        return 0;
    }
    return size > static_cast<off_t>(0xFFFFFFFFULL) ? 0xFFFFFFFFU : static_cast<CARD32>(size);
}

bool AppendRecursiveDirectory(const std::string& root,
                              const std::string& relativePath,
                              unsigned int depth,
                              unsigned int maxDepth,
                              unsigned int maxEntries,
                              std::vector<FileTransferRecursiveEntry>& entries,
                              FileTransferRecursiveSize *size,
                              std::string *reason)
{
    if (entries.size() >= maxEntries) {
        if (size) size->truncated = true;
        return true;
    }
    std::string resolved;
    if (relativePath.empty()) {
        resolved = root;
    } else if (!ResolveFileTransferPath(root, relativePath, resolved, reason)) {
        return false;
    }

    DIR *dir = opendir(resolved.c_str());
    if (!dir) {
        if (reason) *reason = LastSystemError("opendir");
        return false;
    }

    std::vector<std::string> names;
    while (dirent *entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        names.push_back(name);
    }
    closedir(dir);
    std::sort(names.begin(), names.end());

    for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it) {
        if (entries.size() >= maxEntries) {
            if (size) size->truncated = true;
            return true;
        }
        const std::string childRelative = relativePath.empty() ? *it : relativePath + "/" + *it;
        const std::string childPath = JoinRootPath(resolved, *it);
        struct stat st;
        FileTransferRecursiveEntry item;
        item.relativePath = childRelative;
        item.directory = false;
        item.size = 0;
        item.inaccessible = false;
        if (lstat(childPath.c_str(), &st) != 0 || S_ISLNK(st.st_mode)) {
            item.inaccessible = true;
        } else {
            item.directory = S_ISDIR(st.st_mode);
            if (S_ISREG(st.st_mode)) {
                item.size = SaturatingFileSize(st.st_size);
            }
        }
        entries.push_back(item);
        if (size && !item.inaccessible) {
            if (item.directory) {
                size->directories += 1;
            } else {
                size->files += 1;
                const CARD32 before = size->bytesLow;
                size->bytesLow += item.size;
                if (size->bytesLow < before) {
                    size->truncated = true;
                    size->bytesLow = 0xFFFFFFFFU;
                }
            }
        }
        if (item.directory && !item.inaccessible && depth < maxDepth) {
            if (!AppendRecursiveDirectory(root, childRelative, depth + 1, maxDepth, maxEntries, entries, size, reason)) {
                return false;
            }
        }
    }
    return true;
}
#endif

bool IsDirectoryPath(const std::string& path, std::string *reason)
{
#ifndef _WIN32
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        if (reason) *reason = LastSystemError("stat");
        return false;
    }
    if (S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode)) {
        if (reason) *reason = "file-transfer path is not a directory";
        return false;
    }
    return true;
#else
    (void)path;
    if (reason) *reason = "portable file-transfer filesystem helpers are not used on Windows builds";
    return false;
#endif
}

} // namespace

const char *FileTransferModeName(FileTransferMode mode)
{
    switch (mode) {
    case FileTransferMode::Disabled:
        return "disabled";
    case FileTransferMode::RejectOnly:
        return "reject-only";
    case FileTransferMode::ReadOnly:
        return "read-only";
    case FileTransferMode::ReadWrite:
        return "read-write";
    }
    return "unknown";
}

bool ParseFileTransferMode(const std::string& value, FileTransferMode& mode)
{
    if (value == "disabled" || value == "off") {
        mode = FileTransferMode::Disabled;
        return true;
    }
    if (value == "reject-only") {
        mode = FileTransferMode::RejectOnly;
        return true;
    }
    if (value == "read-only" || value == "readonly") {
        mode = FileTransferMode::ReadOnly;
        return true;
    }
    if (value == "read-write" || value == "readwrite") {
        mode = FileTransferMode::ReadWrite;
        return true;
    }
    return false;
}

CARD32 DefaultFileTransferPayloadLimit()
{
    return 1024U * 1024U;
}


bool FileTransferMessageMayCarryPath(CARD8 contentType)
{
    return contentType == rfbFileTransferRequest ||
           contentType == rfbFileTransferOffer;
}

bool IsSafeFileTransferRelativePath(const std::string& requestedPath, std::string *reason)
{
    if (requestedPath.empty()) {
        if (reason) *reason = "file-transfer path must not be empty";
        return false;
    }
    if (requestedPath[0] == '/' || requestedPath.find('\0') != std::string::npos) {
        if (reason) *reason = "file-transfer path must be a relative path";
        return false;
    }
    std::size_t start = 0;
    while (start <= requestedPath.size()) {
        const std::size_t slash = requestedPath.find('/', start);
        const std::string part = requestedPath.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (part.empty() || part == "." || part == "..") {
            if (reason) *reason = "file-transfer path contains an unsafe component";
            return false;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    if (reason) reason->clear();
    return true;
}

bool ResolveFileTransferPath(const std::string& root, const std::string& requestedPath, std::string& resolvedPath, std::string *reason)
{
    if (root.empty() || root[0] != '/') {
        if (reason) *reason = "file-transfer root must be an absolute path";
        return false;
    }
    if (!IsSafeFileTransferRelativePath(requestedPath, reason)) {
        return false;
    }
    resolvedPath = root;
    if (!resolvedPath.empty() && resolvedPath[resolvedPath.size() - 1] != '/') {
        resolvedPath += '/';
    }
    resolvedPath += requestedPath;
    if (reason) reason->clear();
    return true;
}

FileTransferDecision EvaluateFileTransferMessage(const FileTransferMessage& message,
                                                 FileTransferMode mode,
                                                 CARD32 payloadLimitBytes)
{
    FileTransferDecision decision;
    decision.accepted = false;
    decision.readPayload = true;
    decision.payloadBytes = message.length;
    decision.abortReason = message.contentParam;
    decision.reason.clear();

    if (message.length > payloadLimitBytes) {
        decision.readPayload = false;
        decision.payloadBytes = 0;
        decision.reason = "file-transfer payload exceeds configured guard limit";
        return decision;
    }

    if (mode == FileTransferMode::Disabled) {
        decision.reason = "file transfer is disabled";
        return decision;
    }

    if (mode == FileTransferMode::RejectOnly) {
        decision.reason = "file transfer runtime is configured for safe reject-only mode";
        return decision;
    }

    decision.reason = "file transfer runtime requires message-specific handling";
    decision.accepted = true;
    return decision;
}

bool ListFileTransferDirectory(const std::string& root,
                               const std::string& requestedPath,
                               std::vector<FileTransferDirectoryEntry>& entries,
                               std::string *reason)
{
    entries.clear();
#ifndef _WIN32
    std::string resolved;
    if (requestedPath.empty()) {
        resolved = root;
    } else if (!ResolveFileTransferPath(root, requestedPath, resolved, reason)) {
        return false;
    }
    if (!IsDirectoryPath(resolved, reason)) {
        return false;
    }
    DIR *dir = opendir(resolved.c_str());
    if (!dir) {
        if (reason) *reason = LastSystemError("opendir");
        return false;
    }
    while (dirent *entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == ".") {
            continue;
        }
        const std::string fullPath = JoinRootPath(resolved, name);
        struct stat st;
        FileTransferDirectoryEntry item;
        item.name = name;
        item.directory = false;
        item.size = 0;
        item.inaccessible = false;
        if (lstat(fullPath.c_str(), &st) != 0) {
            item.inaccessible = true;
        } else if (S_ISLNK(st.st_mode)) {
            // Do not follow symlinks from a file-transfer root; expose them as inaccessible entries.
            item.inaccessible = true;
        } else {
            item.directory = S_ISDIR(st.st_mode);
            if (S_ISREG(st.st_mode) && st.st_size > 0) {
                item.size = st.st_size > static_cast<off_t>(0xFFFFFFFFULL) ? 0xFFFFFFFFU : static_cast<CARD32>(st.st_size);
            }
        }
        entries.push_back(item);
    }
    closedir(dir);
    if (reason) reason->clear();
    return true;
#else
    (void)root;
    (void)requestedPath;
    if (reason) *reason = "portable directory listing is not used on Windows builds";
    return false;
#endif
}


bool ListFileTransferDirectoryRecursive(const std::string& root,
                                        const std::string& requestedPath,
                                        unsigned int maxDepth,
                                        unsigned int maxEntries,
                                        std::vector<FileTransferRecursiveEntry>& entries,
                                        std::string *reason)
{
    entries.clear();
    if (maxDepth == 0 || maxEntries == 0) {
        if (reason) *reason = "recursive directory limits must be non-zero";
        return false;
    }
#ifndef _WIN32
    std::string resolved;
    if (requestedPath.empty()) {
        resolved = root;
    } else if (!ResolveFileTransferPath(root, requestedPath, resolved, reason)) {
        return false;
    }
    if (!IsDirectoryPath(resolved, reason)) {
        return false;
    }
    const std::string relativeRoot = requestedPath == "." ? std::string() : requestedPath;
    const bool ok = AppendRecursiveDirectory(root, relativeRoot, 1, maxDepth, maxEntries, entries, nullptr, reason);
    if (ok && reason) reason->clear();
    return ok;
#else
    (void)root;
    (void)requestedPath;
    (void)maxDepth;
    (void)maxEntries;
    if (reason) *reason = "portable recursive directory listing is not used on Windows builds";
    return false;
#endif
}

bool MeasureFileTransferDirectoryRecursive(const std::string& root,
                                           const std::string& requestedPath,
                                           unsigned int maxDepth,
                                           unsigned int maxEntries,
                                           FileTransferRecursiveSize& size,
                                           std::string *reason)
{
    size.files = 0;
    size.directories = 0;
    size.bytesLow = 0;
    size.truncated = false;
    std::vector<FileTransferRecursiveEntry> entries;
#ifndef _WIN32
    const bool ok = ListFileTransferDirectoryRecursive(root, requestedPath, maxDepth, maxEntries, entries, reason);
    if (!ok) {
        return false;
    }
    // Re-run with accounting to avoid exposing partially measured state when listing fails.
    entries.clear();
    const std::string relativeRoot = requestedPath == "." ? std::string() : requestedPath;
    if (!AppendRecursiveDirectory(root, relativeRoot, 1, maxDepth, maxEntries, entries, &size, reason)) {
        return false;
    }
    if (reason) reason->clear();
    return true;
#else
    (void)entries;
    (void)root;
    (void)requestedPath;
    (void)maxDepth;
    (void)maxEntries;
    if (reason) *reason = "portable recursive directory measurement is not used on Windows builds";
    return false;
#endif
}

bool PrepareFileTransferUpload(const std::string& root,
                               const std::string& requestedPath,
                               std::string& finalPath,
                               std::string& temporaryPath,
                               std::string *reason)
{
    if (!ResolveFileTransferPath(root, requestedPath, finalPath, reason)) {
        return false;
    }
#ifndef _WIN32
    temporaryPath = finalPath + ".uvnc-upload.tmp";
    if (unlink(temporaryPath.c_str()) != 0 && errno != ENOENT) {
        if (reason) *reason = LastSystemError("remove stale upload temp file");
        return false;
    }
    std::ofstream out(temporaryPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        if (reason) *reason = "could not create upload temp file";
        return false;
    }
    out.close();
    if (reason) reason->clear();
    return true;
#else
    if (reason) *reason = "portable atomic upload is not used on Windows builds";
    return false;
#endif
}

bool CommitFileTransferUpload(const std::string& temporaryPath,
                              const std::string& finalPath,
                              std::string *reason)
{
#ifndef _WIN32
    if (temporaryPath.empty() || finalPath.empty()) {
        if (reason) *reason = "upload state is incomplete";
        return false;
    }
    if (rename(temporaryPath.c_str(), finalPath.c_str()) != 0) {
        if (reason) *reason = LastSystemError("rename upload temp file");
        return false;
    }
    if (reason) reason->clear();
    return true;
#else
    (void)temporaryPath;
    (void)finalPath;
    if (reason) *reason = "portable atomic upload is not used on Windows builds";
    return false;
#endif
}

void AbortFileTransferUpload(const std::string& temporaryPath)
{
#ifndef _WIN32
    if (!temporaryPath.empty()) {
        unlink(temporaryPath.c_str());
    }
#else
    (void)temporaryPath;
#endif
}

FileTransferCommandResult ExecuteFileTransferCommand(const std::string& root,
                                                     CARD16 command,
                                                     const std::string& payload,
                                                     FileTransferMode mode)
{
    if (mode != FileTransferMode::ReadWrite) {
        return MakeCommandResult(rfbCommandReturn, payload, false, "file-transfer command requires read-write mode");
    }

    std::string resolved;
    std::string reason;
    switch (command) {
    case rfbCDirCreate:
        if (!ResolveFileTransferPath(root, payload, resolved, &reason)) return MakeCommandResult(rfbADirCreate, payload, false, reason);
#ifndef _WIN32
        return MakeCommandResult(rfbADirCreate, payload, mkdir(resolved.c_str(), 0700) == 0, LastSystemError("mkdir"));
#else
        return MakeCommandResult(rfbADirCreate, payload, false, "directory create not implemented on Windows portable path");
#endif
    case rfbCFileCreate:
        if (!ResolveFileTransferPath(root, payload, resolved, &reason)) return MakeCommandResult(rfbAFileCreate, payload, false, reason);
        {
            std::ofstream out(resolved.c_str(), std::ios::binary | std::ios::app);
            return MakeCommandResult(rfbAFileCreate, payload, static_cast<bool>(out), "create file failed");
        }
    case rfbCFileDelete:
        if (!ResolveFileTransferPath(root, payload, resolved, &reason)) return MakeCommandResult(rfbAFileDelete, payload, false, reason);
        if (!IsRegularFilePath(resolved, &reason)) return MakeCommandResult(rfbAFileDelete, payload, false, reason);
#ifndef _WIN32
        return MakeCommandResult(rfbAFileDelete, payload, unlink(resolved.c_str()) == 0, LastSystemError("unlink"));
#else
        return MakeCommandResult(rfbAFileDelete, payload, false, "file delete not implemented on Windows portable path");
#endif
    case rfbCDirDelete:
        if (!ResolveFileTransferPath(root, payload, resolved, &reason)) return MakeCommandResult(rfbADirDelete, payload, false, reason);
        if (!IsDirectoryPath(resolved, &reason)) return MakeCommandResult(rfbADirDelete, payload, false, reason);
#ifndef _WIN32
        return MakeCommandResult(rfbADirDelete, payload, rmdir(resolved.c_str()) == 0, LastSystemError("rmdir"));
#else
        return MakeCommandResult(rfbADirDelete, payload, false, "directory delete not implemented on Windows portable path");
#endif
    case rfbCFileRename:
    case rfbCDirRename: {
        std::string from;
        std::string to;
        if (!SplitRenamePayload(payload, from, to, &reason)) {
            return MakeCommandResult(command == rfbCFileRename ? rfbAFileRename : rfbADirRename, payload, false, reason);
        }
        std::string resolvedFrom;
        std::string resolvedTo;
        if (!ResolveFileTransferPath(root, from, resolvedFrom, &reason) || !ResolveFileTransferPath(root, to, resolvedTo, &reason)) {
            return MakeCommandResult(command == rfbCFileRename ? rfbAFileRename : rfbADirRename, payload, false, reason);
        }
        if (command == rfbCFileRename && !IsRegularFilePath(resolvedFrom, &reason)) {
            return MakeCommandResult(rfbAFileRename, payload, false, reason);
        }
        if (command == rfbCDirRename && !IsDirectoryPath(resolvedFrom, &reason)) {
            return MakeCommandResult(rfbADirRename, payload, false, reason);
        }
#ifndef _WIN32
        return MakeCommandResult(command == rfbCFileRename ? rfbAFileRename : rfbADirRename, payload, rename(resolvedFrom.c_str(), resolvedTo.c_str()) == 0, LastSystemError("rename"));
#else
        return MakeCommandResult(command == rfbCFileRename ? rfbAFileRename : rfbADirRename, payload, false, "rename not implemented on Windows portable path");
#endif
    }
    default:
        return MakeCommandResult(rfbCommandReturn, payload, false, "unsupported file-transfer command");
    }
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
