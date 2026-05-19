// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VIEWER_FILE_TRANSFER_H
#define UVNC_VNCVIEWER_PORTABLE_VIEWER_FILE_TRANSFER_H

#include "rfb.h"
#include "vncPortableTcp.h"
#include "vncPortableRfbTransport.h"

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

struct ViewerFileTransferEntry {
    ViewerFileTransferEntry();

    std::string name;
    bool directory;
    bool inaccessible;
    CARD32 size;
};

struct ViewerFileDownload {
    ViewerFileDownload();

    std::string name;
    CARD32 expectedSize;
    std::vector<CARD8> payload;
};

std::vector<CARD8> EncodeViewerFileTransferRequest(CARD8 contentType,
                                                   CARD16 contentParam,
                                                   CARD32 size,
                                                   const std::string& payload);
bool ReadViewerDirectoryListing(uvnc::winvnc::portable::RfbTransport& transport,
                                std::vector<ViewerFileTransferEntry>& entries,
                                std::string *error = nullptr);
bool RequestViewerDirectoryListing(uvnc::winvnc::portable::RfbTransport& transport,
                                   const std::string& path,
                                   std::vector<ViewerFileTransferEntry>& entries,
                                   std::string *error = nullptr);
bool RequestViewerDrivesList(uvnc::winvnc::portable::RfbTransport& transport,
                             std::vector<ViewerFileTransferEntry>& entries,
                             std::string *error = nullptr);
bool RequestViewerFileDownload(uvnc::winvnc::portable::RfbTransport& transport,
                               const std::string& path,
                               ViewerFileDownload& download,
                               std::string *error = nullptr);
bool RequestViewerFileChecksums(uvnc::winvnc::portable::RfbTransport& transport,
                                const std::string& path,
                                std::vector<std::string>& checksums,
                                std::string *error = nullptr);

bool ReadViewerDirectoryListing(uvnc::winvnc::portable::TcpSocket& socket,
                                std::vector<ViewerFileTransferEntry>& entries,
                                std::string *error = nullptr);
bool RequestViewerDirectoryListing(uvnc::winvnc::portable::TcpSocket& socket,
                                   const std::string& path,
                                   std::vector<ViewerFileTransferEntry>& entries,
                                   std::string *error = nullptr);
bool RequestViewerDrivesList(uvnc::winvnc::portable::TcpSocket& socket,
                             std::vector<ViewerFileTransferEntry>& entries,
                             std::string *error = nullptr);
bool RequestViewerFileDownload(uvnc::winvnc::portable::TcpSocket& socket,
                               const std::string& path,
                               ViewerFileDownload& download,
                               std::string *error = nullptr);
bool RequestViewerFileChecksums(uvnc::winvnc::portable::TcpSocket& socket,
                                const std::string& path,
                                std::vector<std::string>& checksums,
                                std::string *error = nullptr);

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_FILE_TRANSFER_H
