// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFileTransfer.h"

#include <sstream>

namespace uvnc {
namespace winvnc {
namespace portable {

const char *FileTransferModeName(FileTransferMode mode)
{
    switch (mode) {
    case FileTransferMode::Disabled:
        return "disabled";
    case FileTransferMode::RejectOnly:
        return "reject-only";
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
    return false;
}

CARD32 DefaultFileTransferPayloadLimit()
{
    return 1024U * 1024U;
}


bool FileTransferMessageMayCarryPath(CARD8 contentType)
{
    return contentType == rfbDirContentRequest ||
           contentType == rfbFileTransferRequest ||
           contentType == rfbFileTransferOffer ||
           contentType == rfbCommand;
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

    decision.reason = "file transfer runtime only supports safe reject-only mode";
    return decision;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
