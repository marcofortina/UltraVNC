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

namespace uvnc {
namespace winvnc {
namespace portable {

enum class FileTransferMode {
    Disabled,
    RejectOnly,
};

struct FileTransferDecision {
    bool accepted;
    bool readPayload;
    CARD32 payloadBytes;
    CARD16 abortReason;
    std::string reason;
};

const char *FileTransferModeName(FileTransferMode mode);
bool ParseFileTransferMode(const std::string& value, FileTransferMode& mode);
CARD32 DefaultFileTransferPayloadLimit();
FileTransferDecision EvaluateFileTransferMessage(const FileTransferMessage& message,
                                                 FileTransferMode mode,
                                                 CARD32 payloadLimitBytes = DefaultFileTransferPayloadLimit());

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_FILE_TRANSFER_H
