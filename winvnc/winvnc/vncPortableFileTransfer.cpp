// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableFileTransfer.h"

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
