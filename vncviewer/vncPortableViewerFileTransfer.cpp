// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerFileTransfer.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

bool WritePacket(uvnc::winvnc::portable::RfbTransport& transport,
                 CARD8 contentType,
                 CARD16 contentParam,
                 CARD32 size,
                 const std::string& payload)
{
    const std::vector<CARD8> bytes = EncodeViewerFileTransferRequest(contentType, contentParam, size, payload);
    return transport.WriteAll(bytes.data(), bytes.size());
}

bool DrainBytes(uvnc::winvnc::portable::RfbTransport& transport, CARD32 length)
{
    std::vector<CARD8> buffer(length);
    return length == 0 || transport.ReadExact(buffer.data(), buffer.size());
}

bool DrainServerCutText(uvnc::winvnc::portable::RfbTransport& transport, std::string *error)
{
    rfbServerCutTextMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbServerCutText;
    if (!transport.ReadExact(reinterpret_cast<char *>(&message) + 1, sz_rfbServerCutTextMsg - 1)) {
        SetError(error, "failed to read RFB ServerCutText while waiting for file-transfer data");
        return false;
    }
    const int32_t signedLength = static_cast<int32_t>(Swap32IfLE(message.length));
    const CARD32 length = signedLength < 0 ? static_cast<CARD32>(-signedLength) : static_cast<CARD32>(signedLength);
    if (!DrainBytes(transport, length)) {
        SetError(error, "failed to drain RFB ServerCutText while waiting for file-transfer data");
        return false;
    }
    return true;
}


bool DrainFramebufferUpdate(uvnc::winvnc::portable::RfbTransport& transport, std::string *error)
{
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    if (!transport.ReadExact(reinterpret_cast<char *>(&update) + 1, sz_rfbFramebufferUpdateMsg - 1)) {
        SetError(error, "failed to read framebuffer update while waiting for file-transfer data");
        return false;
    }
    const CARD16 rects = Swap16IfLE(update.nRects);
    for (CARD16 i = 0; i < rects; ++i) {
        rfbFramebufferUpdateRectHeader rect;
        if (!transport.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
            SetError(error, "failed to read framebuffer update rectangle while waiting for file-transfer data");
            return false;
        }
        const CARD32 encoding = Swap32IfLE(rect.encoding);
        const CARD16 width = Swap16IfLE(rect.r.w);
        const CARD16 height = Swap16IfLE(rect.r.h);
        if (encoding == rfbEncodingLastRect || encoding == rfbEncodingPointerPos || encoding == rfbEncodingNewFBSize) {
            continue;
        }
        if (encoding == rfbEncodingRichCursor) {
            const CARD32 mask = ((width + 7) / 8) * height;
            if (!DrainBytes(transport, static_cast<CARD32>(width) * height * 4 + mask)) {
                SetError(error, "failed to drain RichCursor while waiting for file-transfer data");
                return false;
            }
            continue;
        }
        if (encoding == rfbEncodingXCursor) {
            const CARD32 plane = ((width + 7) / 8) * height;
            if (!DrainBytes(transport, sz_rfbXCursorColors + plane + plane)) {
                SetError(error, "failed to drain XCursor while waiting for file-transfer data");
                return false;
            }
            continue;
        }
        SetError(error, "framebuffer update contained drawable data while waiting for file-transfer data");
        return false;
    }
    return true;
}

bool ReadPacket(uvnc::winvnc::portable::RfbTransport& transport,
                rfbFileTransferMsg& message,
                std::vector<CARD8>& payload,
                std::string *error)
{
    for (;;) {
        CARD8 type = 0;
        if (!transport.ReadExact(&type, sizeof(type))) {
            SetError(error, "failed to read RFB message while waiting for file-transfer data");
            return false;
        }
        if (type == rfbBell) {
            continue;
        }
        if (type == rfbServerCutText) {
            if (!DrainServerCutText(transport, error)) {
                return false;
            }
            continue;
        }
        if (type == rfbFramebufferUpdate) {
            if (!DrainFramebufferUpdate(transport, error)) {
                return false;
            }
            continue;
        }
        if (type != rfbFileTransfer) {
            SetError(error, "unexpected RFB message while reading file-transfer data");
            return false;
        }
        std::memset(&message, 0, sizeof(message));
        message.type = type;
        if (!transport.ReadExact(reinterpret_cast<char *>(&message) + 1, sz_rfbFileTransferMsg - 1)) {
            SetError(error, "failed to read RFB file-transfer header");
            return false;
        }
        const CARD32 length = Swap32IfLE(message.length);
        payload.assign(length, 0);
        if (length > 0 && !transport.ReadExact(payload.data(), payload.size())) {
            SetError(error, "failed to read RFB file-transfer payload");
            return false;
        }
        return true;
    }
}

std::string PayloadString(const std::vector<CARD8>& payload)
{
    return std::string(reinterpret_cast<const char *>(payload.data()), payload.size());
}

} // namespace

ViewerFileTransferEntry::ViewerFileTransferEntry()
    : name(), directory(false), inaccessible(false), size(0)
{
}

ViewerFileDownload::ViewerFileDownload()
    : name(), expectedSize(0), payload()
{
}

std::vector<CARD8> EncodeViewerFileTransferRequest(CARD8 contentType,
                                                   CARD16 contentParam,
                                                   CARD32 size,
                                                   const std::string& payload)
{
    rfbFileTransferMsg message;
    std::memset(&message, 0, sizeof(message));
    message.type = rfbFileTransfer;
    message.contentType = contentType;
    message.contentParam = Swap16IfLE(contentParam);
    message.size = Swap32IfLE(size);
    message.length = Swap32IfLE(static_cast<CARD32>(payload.size()));

    std::vector<CARD8> bytes(sz_rfbFileTransferMsg + payload.size());
    std::memcpy(bytes.data(), &message, sz_rfbFileTransferMsg);
    if (!payload.empty()) {
        std::memcpy(bytes.data() + sz_rfbFileTransferMsg, payload.data(), payload.size());
    }
    return bytes;
}

bool ReadViewerDirectoryListing(uvnc::winvnc::portable::RfbTransport& transport,
                                std::vector<ViewerFileTransferEntry>& entries,
                                std::string *error)
{
    entries.clear();
    while (true) {
        rfbFileTransferMsg message;
        std::vector<CARD8> payload;
        if (!ReadPacket(transport, message, payload, error)) {
            return false;
        }
        if (message.contentType != rfbDirPacket) {
            SetError(error, "expected RFB directory packet");
            return false;
        }
        const CARD16 param = Swap16IfLE(message.contentParam);
        const CARD32 length = Swap32IfLE(message.length);
        if (param == 0 && length == 0) {
            return true;
        }
        ViewerFileTransferEntry entry;
        entry.name = PayloadString(payload);
        entry.size = Swap32IfLE(message.size);
        entry.directory = param == rfbADirectory || param == rfbADrivesList;
        entry.inaccessible = param == rfbADirInaccessible;
        entries.push_back(entry);
    }
}

bool RequestViewerDirectoryListing(uvnc::winvnc::portable::RfbTransport& transport,
                                   const std::string& path,
                                   std::vector<ViewerFileTransferEntry>& entries,
                                   std::string *error)
{
    if (!WritePacket(transport, rfbDirContentRequest, rfbRDirContent, 0, path)) {
        SetError(error, "failed to request RFB directory listing");
        return false;
    }
    return ReadViewerDirectoryListing(transport, entries, error);
}

bool RequestViewerDrivesList(uvnc::winvnc::portable::RfbTransport& transport,
                             std::vector<ViewerFileTransferEntry>& entries,
                             std::string *error)
{
    if (!WritePacket(transport, rfbDirContentRequest, rfbRDrivesList, 0, std::string())) {
        SetError(error, "failed to request RFB drives list");
        return false;
    }
    return ReadViewerDirectoryListing(transport, entries, error);
}

bool RequestViewerFileDownload(uvnc::winvnc::portable::RfbTransport& transport,
                               const std::string& path,
                               ViewerFileDownload& download,
                               std::string *error)
{
    download = ViewerFileDownload();
    if (!WritePacket(transport, rfbFileTransferRequest, 0, 0, path)) {
        SetError(error, "failed to request RFB file download");
        return false;
    }

    bool sawHeader = false;
    while (true) {
        rfbFileTransferMsg message;
        std::vector<CARD8> payload;
        if (!ReadPacket(transport, message, payload, error)) {
            return false;
        }
        const CARD32 size = Swap32IfLE(message.size);
        if (message.contentType == rfbFileHeader) {
            sawHeader = true;
            download.name = PayloadString(payload);
            download.expectedSize = size;
            continue;
        }
        if (message.contentType == rfbFilePacket) {
            if (!sawHeader) {
                SetError(error, "file packet received before file header");
                return false;
            }
            const CARD32 offset = size;
            if (download.payload.size() != offset) {
                SetError(error, "non-contiguous RFB file packet offset");
                return false;
            }
            download.payload.insert(download.payload.end(), payload.begin(), payload.end());
            continue;
        }
        if (message.contentType == rfbEndOfFile) {
            if (!sawHeader || download.payload.size() != download.expectedSize) {
                SetError(error, "RFB file download ended with unexpected size");
                return false;
            }
            return true;
        }
        if (message.contentType == rfbAbortFileTransfer) {
            SetError(error, "RFB file download was aborted by the server");
            return false;
        }
        SetError(error, "unexpected RFB file-transfer packet during download");
        return false;
    }
}

bool RequestViewerFileChecksums(uvnc::winvnc::portable::RfbTransport& transport,
                                const std::string& path,
                                std::vector<std::string>& checksums,
                                std::string *error)
{
    checksums.clear();
    if (!WritePacket(transport, rfbFileChecksums, 0, 0, path)) {
        SetError(error, "failed to request RFB file checksums");
        return false;
    }
    rfbFileTransferMsg message;
    std::vector<CARD8> payload;
    if (!ReadPacket(transport, message, payload, error)) {
        return false;
    }
    if (message.contentType != rfbFileChecksums) {
        SetError(error, "expected RFB file checksum response");
        return false;
    }
    std::istringstream input(PayloadString(payload));
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            checksums.push_back(line);
        }
    }
    return true;
}


bool ReadViewerDirectoryListing(uvnc::winvnc::portable::TcpSocket& socket,
                                std::vector<ViewerFileTransferEntry>& entries,
                                std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return ReadViewerDirectoryListing(transport, entries, error);
}

bool RequestViewerDirectoryListing(uvnc::winvnc::portable::TcpSocket& socket,
                                   const std::string& path,
                                   std::vector<ViewerFileTransferEntry>& entries,
                                   std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return RequestViewerDirectoryListing(transport, path, entries, error);
}

bool RequestViewerDrivesList(uvnc::winvnc::portable::TcpSocket& socket,
                             std::vector<ViewerFileTransferEntry>& entries,
                             std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return RequestViewerDrivesList(transport, entries, error);
}

bool RequestViewerFileDownload(uvnc::winvnc::portable::TcpSocket& socket,
                               const std::string& path,
                               ViewerFileDownload& download,
                               std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return RequestViewerFileDownload(transport, path, download, error);
}

bool RequestViewerFileChecksums(uvnc::winvnc::portable::TcpSocket& socket,
                                const std::string& path,
                                std::vector<std::string>& checksums,
                                std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return RequestViewerFileChecksums(transport, path, checksums, error);
}

bool UploadViewerFile(uvnc::winvnc::portable::TcpSocket& socket,
                      const std::string& remotePath,
                      const std::vector<CARD8>& payload,
                      std::string *error)
{
    uvnc::winvnc::portable::TcpRfbTransport transport(socket);
    return UploadViewerFile(transport, remotePath, payload, error);
}

bool UploadViewerFile(uvnc::winvnc::portable::RfbTransport& transport,
                      const std::string& remotePath,
                      const std::vector<CARD8>& payload,
                      std::string *error)
{
    if (remotePath.empty()) {
        SetError(error, "remote upload path must not be empty");
        return false;
    }
    if (!WritePacket(transport, rfbFileTransferOffer, 0, static_cast<CARD32>(payload.size()), remotePath)) {
        SetError(error, "failed to offer RFB file upload");
        return false;
    }
    rfbFileTransferMsg response;
    std::vector<CARD8> responsePayload;
    if (!ReadPacket(transport, response, responsePayload, error)) {
        return false;
    }
    if (response.contentType != rfbFileAcceptHeader) {
        SetError(error, "RFB server did not accept file upload");
        return false;
    }
    CARD32 offset = 0;
    while (offset < payload.size()) {
        const CARD32 chunk = static_cast<CARD32>(std::min<std::size_t>(payload.size() - offset, sz_rfbBlockSize));
        std::string bytes(reinterpret_cast<const char *>(payload.data() + offset), chunk);
        if (!WritePacket(transport, rfbFilePacket, 0, offset, bytes)) {
            SetError(error, "failed to write RFB file upload packet");
            return false;
        }
        offset += chunk;
    }
    if (!WritePacket(transport, rfbEndOfFile, 0, static_cast<CARD32>(payload.size()), std::string())) {
        SetError(error, "failed to finish RFB file upload");
        return false;
    }
    if (error) error->clear();
    return true;
}


} // namespace portable
} // namespace vncviewer
} // namespace uvnc
