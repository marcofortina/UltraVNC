// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerSession.h"

#include "vncPortableViewerSecurity.h"
#include "vncPortableViewerFileTransfer.h"
#include "vncPortableVncAuth.h"

#include "vncPortableExtendedClipboard.h"

#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"
#include "vncPortableRfbTransport.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include <zlib.h>

extern "C" {
#include <jpeglib.h>
}

namespace uvnc {
namespace vncviewer {
namespace portable {
namespace {

using uvnc::winvnc::portable::AuthOkValue;
using uvnc::winvnc::portable::EncodeClientCutText;
using uvnc::winvnc::portable::EncodeFramebufferUpdateRequest;
using uvnc::winvnc::portable::EncodeKeyEvent;
using uvnc::winvnc::portable::EncodePointerEvent;
using uvnc::winvnc::portable::EncodeSetEncodings;
using uvnc::winvnc::portable::EncodeExtendedClientCutText;
using uvnc::winvnc::portable::EncodeExtendedClipboardCaps;
using uvnc::winvnc::portable::DecodeExtendedClipboardPayload;
using uvnc::winvnc::portable::ExtendedClipboardPayload;
using uvnc::winvnc::portable::ExtendedClipboardPayloadLength;
using uvnc::winvnc::portable::IsExtendedClipboardWireLength;
using uvnc::winvnc::portable::IsProtocolVersionMessage;
using uvnc::winvnc::portable::ProtocolVersion38;
using uvnc::winvnc::portable::kVeNCryptVersion;
using uvnc::winvnc::portable::kVeNCryptSubTypeX509Vnc;
using uvnc::winvnc::portable::KeyEvent;
using uvnc::winvnc::portable::PointerEvent;
using uvnc::winvnc::portable::TcpSocket;
using uvnc::winvnc::portable::RfbTransport;
using uvnc::winvnc::portable::TcpRfbTransport;
using uvnc::winvnc::portable::CreateOpenSslClientTransport;

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

void SetUnsupportedEncodingError(std::string *error, CARD32 encoding)
{
    std::ostringstream message;
    message << "unsupported RFB framebuffer update encoding: " << encoding;
    SetError(error, message.str());
}

bool ReadServerInit(RfbTransport& socket, ViewerSessionResult& result, std::string *error)
{
    rfbServerInitMsg init;
    if (!socket.ReadExact(&init, sz_rfbServerInitMsg)) {
        SetError(error, "failed to read RFB ServerInit");
        return false;
    }

    result.width = Swap16IfLE(init.framebufferWidth);
    result.height = Swap16IfLE(init.framebufferHeight);
    result.format = init.format;
    result.format.redMax = Swap16IfLE(result.format.redMax);
    result.format.greenMax = Swap16IfLE(result.format.greenMax);
    result.format.blueMax = Swap16IfLE(result.format.blueMax);

    const CARD32 nameLength = Swap32IfLE(init.nameLength);
    result.desktopName.assign(nameLength, '\0');
    if (nameLength > 0 && !socket.ReadExact(&result.desktopName[0], nameLength)) {
        SetError(error, "failed to read RFB desktop name");
        return false;
    }
    return true;
}

bool RunViewerVeNCryptX509VncNegotiation(RfbTransport& transport, std::string *error)
{
    CARD16 serverVersion = 0;
    if (!transport.ReadExact(&serverVersion, sizeof(serverVersion)) || Swap16IfLE(serverVersion) != kVeNCryptVersion) {
        SetError(error, "failed to negotiate VeNCrypt version");
        return false;
    }
    const CARD16 clientVersion = Swap16IfLE(static_cast<CARD16>(kVeNCryptVersion));
    if (!transport.WriteAll(&clientVersion, sizeof(clientVersion))) {
        SetError(error, "failed to write VeNCrypt version");
        return false;
    }
    CARD8 versionStatus = 1;
    if (!transport.ReadExact(&versionStatus, sizeof(versionStatus)) || versionStatus != 0) {
        SetError(error, "VeNCrypt version rejected by server");
        return false;
    }
    CARD8 subtypeCount = 0;
    if (!transport.ReadExact(&subtypeCount, sizeof(subtypeCount)) || subtypeCount == 0) {
        SetError(error, "VeNCrypt server reported no subtypes");
        return false;
    }
    bool supportsX509Vnc = false;
    for (CARD8 i = 0; i < subtypeCount; ++i) {
        CARD32 subtype = 0;
        if (!transport.ReadExact(&subtype, sizeof(subtype))) {
            SetError(error, "failed to read VeNCrypt subtype");
            return false;
        }
        if (Swap32IfLE(subtype) == kVeNCryptSubTypeX509Vnc) {
            supportsX509Vnc = true;
        }
    }
    if (!supportsX509Vnc) {
        SetError(error, "VeNCrypt server does not offer X509Vnc");
        return false;
    }
    const CARD32 selectedSubtype = Swap32IfLE(kVeNCryptSubTypeX509Vnc);
    if (!transport.WriteAll(&selectedSubtype, sizeof(selectedSubtype))) {
        SetError(error, "failed to select VeNCrypt X509Vnc");
        return false;
    }
    CARD8 accepted = 0;
    if (!transport.ReadExact(&accepted, sizeof(accepted)) || accepted != 1) {
        SetError(error, "VeNCrypt X509Vnc subtype rejected by server");
        return false;
    }
    return true;
}

bool RunVncAuthOnTransport(RfbTransport& socket, const ViewerConfig& config, std::string *error)
{
    std::vector<unsigned char> challenge(16);
    if (!socket.ReadExact(challenge.data(), challenge.size())) {
        SetError(error, "failed to read RFB VNCAuth challenge");
        return false;
    }
    std::vector<unsigned char> response;
    if (!EncryptVncAuthChallenge(challenge, config.Password(), response, error)) {
        return false;
    }
    if (!socket.WriteAll(response.data(), response.size())) {
        SetError(error, "failed to write RFB VNCAuth response");
        return false;
    }
    return true;
}

bool FinishRfbAuthResult(RfbTransport& socket, CARD8 selectedSecurity, std::string *error)
{
    CARD32 authResult = 1;
    if (!socket.ReadExact(&authResult, sizeof(authResult)) || Swap32IfLE(authResult) != rfbVncAuthOK) {
        SetError(error, selectedSecurity == rfbVncAuth ? "RFB VNCAuth security failed" : "RFB no-auth security failed");
        return false;
    }
    return true;
}

bool SendClientInitAndReadServerInit(RfbTransport& socket, const ViewerConfig& config, ViewerSessionResult& result, std::string *error)
{
    rfbClientInitMsg clientInit;
    std::memset(&clientInit, 0, sizeof(clientInit));
    clientInit.flags = config.Shared() ? clientInitShared : clientInitNotShare;
    if (!socket.WriteAll(&clientInit, sz_rfbClientInitMsg)) {
        SetError(error, "failed to write RFB ClientInit");
        return false;
    }
    return ReadServerInit(socket, result, error);
}


bool RunHandshakeOnTransport(RfbTransport& socket, const ViewerConfig& config, ViewerSessionResult& result, std::unique_ptr<RfbTransport> *upgradedTransport, std::string *error)
{
    char serverVersion[sz_rfbProtocolVersionMsg] = {};
    if (!socket.ReadExact(serverVersion, sizeof(serverVersion))) {
        SetError(error, "failed to read RFB server protocol version");
        return false;
    }
    if (!IsProtocolVersionMessage(std::string(serverVersion, sizeof(serverVersion)))) {
        SetError(error, "invalid RFB server protocol version");
        return false;
    }

    const std::string clientVersion = ProtocolVersion38();
    if (!socket.WriteAll(clientVersion.data(), clientVersion.size())) {
        SetError(error, "failed to write RFB client protocol version");
        return false;
    }

    CARD8 securityCount = 0;
    if (!socket.ReadExact(&securityCount, sizeof(securityCount))) {
        SetError(error, "failed to read RFB security type count");
        return false;
    }
    if (securityCount == 0) {
        SetError(error, "RFB server reported no security types");
        return false;
    }
    std::vector<CARD8> securityTypes(securityCount);
    if (!socket.ReadExact(securityTypes.data(), securityTypes.size())) {
        SetError(error, "failed to read RFB security types");
        return false;
    }

    const ViewerSecurityDecision security = SelectViewerSecurityType(securityTypes, !config.Password().empty(), config.AllowNoAuth(), config.TransportSecurity() == ViewerTransportSecurityMode::VeNCryptX509Vnc);
    if (security.selection == ViewerSecuritySelection::Unsupported) {
        SetError(error, security.error);
        return false;
    }
    const CARD8 selectedSecurity = security.wireType;

    if (!socket.WriteAll(&selectedSecurity, sizeof(selectedSecurity))) {
        SetError(error, "failed to select RFB security type");
        return false;
    }

    std::unique_ptr<RfbTransport> tlsTransport;
    RfbTransport *active = &socket;
    if (selectedSecurity == rfbVeNCypt) {
        if (!RunViewerVeNCryptX509VncNegotiation(socket, error)) {
            return false;
        }
        TcpRfbTransport *tcp = dynamic_cast<TcpRfbTransport *>(&socket);
        if (!tcp) {
            SetError(error, "VeNCrypt requires a TCP-backed clear transport");
            return false;
        }
        if (!CreateOpenSslClientTransport(tcp->Socket(), config.TlsCaFile(), config.TlsServerName().empty() ? config.Host() : config.TlsServerName(), config.TlsVerifyPeer(), tlsTransport, error)) {
            return false;
        }
        if (upgradedTransport) {
            *upgradedTransport = std::move(tlsTransport);
            active = upgradedTransport->get();
        } else {
            active = tlsTransport.get();
        }
        if (!RunVncAuthOnTransport(*active, config, error)) {
            return false;
        }
    } else if (selectedSecurity == rfbVncAuth) {
        if (!RunVncAuthOnTransport(*active, config, error)) {
            return false;
        }
    }
    if (!FinishRfbAuthResult(*active, selectedSecurity, error)) {
        return false;
    }
    if (!SendClientInitAndReadServerInit(*active, config, result, error)) {
        return false;
    }
    std::vector<CARD32> encodings;
    for (std::size_t i = 0; i < config.Encodings().size(); ++i) {
        encodings.push_back(static_cast<CARD32>(config.Encodings()[i]));
    }
    const std::vector<CARD8> setEncodings = EncodeSetEncodings(encodings);
    if (!active->WriteAll(setEncodings.data(), setEncodings.size())) {
        SetError(error, "failed to write RFB SetEncodings");
        return false;
    }
    if (std::find(encodings.begin(), encodings.end(), static_cast<CARD32>(rfbEncodingExtendedClipboard)) != encodings.end()) {
        const std::vector<CARD8> caps = EncodeExtendedClientCutText(EncodeExtendedClipboardCaps(clipCaps | clipRequest | clipProvide | clipNotify | clipPeek | clipText));
        if (!active->WriteAll(caps.data(), caps.size())) {
            SetError(error, "failed to write RFB extended clipboard caps");
            return false;
        }
    }
    return true;
}

unsigned int BytesPerPixel(const rfbPixelFormat& format)
{
    return format.bitsPerPixel / 8;
}

bool EnsureFramebufferStorage(ViewerSessionResult& result, std::vector<CARD8>& framebuffer, std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || result.width == 0 || result.height == 0) {
        SetError(error, "invalid RFB framebuffer dimensions");
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(result.width) * result.height * bytesPerPixel;
    if (framebuffer.size() != expected) {
        framebuffer.assign(expected, 0);
    }
    return true;
}


void CopyRectToFramebuffer(ViewerSessionResult& result,
                           std::vector<CARD8>& framebuffer,
                           unsigned int x,
                           unsigned int y,
                           unsigned int width,
                           unsigned int height,
                           const std::vector<CARD8>& pixels)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    for (unsigned int row = 0; row < height; ++row) {
        const std::size_t src = static_cast<std::size_t>(row) * width * bytesPerPixel;
        const std::size_t dst = (static_cast<std::size_t>(y + row) * result.width + x) * bytesPerPixel;
        std::copy(pixels.begin() + src,
                  pixels.begin() + src + static_cast<std::size_t>(width) * bytesPerPixel,
                  framebuffer.begin() + dst);
    }
}

std::vector<CARD8> SolidPixelRect(const std::vector<CARD8>& pixel,
                                  unsigned int width,
                                  unsigned int height,
                                  unsigned int bytesPerPixel)
{
    std::vector<CARD8> pixels(static_cast<std::size_t>(width) * height * bytesPerPixel);
    for (std::size_t offset = 0; offset < pixels.size(); offset += bytesPerPixel) {
        std::copy(pixel.begin(), pixel.end(), pixels.begin() + offset);
    }
    return pixels;
}

bool ReadPixel(RfbTransport& socket, unsigned int bytesPerPixel, std::vector<CARD8>& pixel, std::string *error)
{
    pixel.assign(bytesPerPixel, 0);
    if (!socket.ReadExact(pixel.data(), pixel.size())) {
        SetError(error, "failed to read RFB encoded pixel");
        return false;
    }
    return true;
}

bool ReadCompressedPayload(RfbTransport& socket,
                           std::size_t expectedSize,
                           std::vector<CARD8>& payload,
                           std::string *error)
{
    rfbZlibHeader header;
    if (!socket.ReadExact(&header, sz_rfbZlibHeader)) {
        SetError(error, "failed to read RFB zlib header");
        return false;
    }
    const CARD32 compressedSize = Swap32IfLE(header.nBytes);
    std::vector<CARD8> compressed(compressedSize);
    if (compressedSize > 0 && !socket.ReadExact(compressed.data(), compressed.size())) {
        SetError(error, "failed to read RFB zlib payload");
        return false;
    }
    payload.assign(expectedSize, 0);
    uLongf outputSize = static_cast<uLongf>(payload.size());
    const int rc = uncompress(payload.data(), &outputSize, compressed.data(), static_cast<uLong>(compressed.size()));
    if (rc != Z_OK || outputSize != payload.size()) {
        SetError(error, "failed to decompress RFB zlib payload");
        return false;
    }
    return true;
}


unsigned int ZrleBytesPerPixel(const rfbPixelFormat& format)
{
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    if (bytesPerPixel == 4 && format.trueColour && format.depth <= 24) {
        return 3;
    }
    return bytesPerPixel;
}

std::vector<CARD8> ExpandZrlePixel(const rfbPixelFormat& format, const std::vector<CARD8>& compact)
{
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    if (compact.size() == bytesPerPixel) {
        return compact;
    }
    std::vector<CARD8> pixel(bytesPerPixel, 0);
    if (bytesPerPixel == 4 && compact.size() == 3) {
        if (format.bigEndian) {
            std::copy(compact.begin(), compact.end(), pixel.begin() + 1);
        } else {
            std::copy(compact.begin(), compact.end(), pixel.begin());
        }
    }
    return pixel;
}

bool EnsureZrleInflateStream(z_stream& stream, bool& initialized, std::string *error)
{
    if (initialized) {
        return true;
    }
    std::memset(&stream, 0, sizeof(stream));
    const int rc = inflateInit(&stream);
    if (rc != Z_OK) {
        SetError(error, "failed to initialize RFB ZRLE inflater");
        return false;
    }
    initialized = true;
    return true;
}

bool ReadZrleCompressedPayload(RfbTransport& socket,
                               z_stream& stream,
                               bool& streamInitialized,
                               std::vector<CARD8>& payload,
                               std::string *error)
{
    rfbZRLEHeader header;
    if (!socket.ReadExact(&header, sz_rfbZRLEHeader)) {
        SetError(error, "failed to read RFB ZRLE header");
        return false;
    }
    const CARD32 compressedSize = Swap32IfLE(header.length);
    std::vector<CARD8> compressed(compressedSize);
    if (compressedSize > 0 && !socket.ReadExact(compressed.data(), compressed.size())) {
        SetError(error, "failed to read RFB ZRLE payload");
        return false;
    }
    if (!EnsureZrleInflateStream(stream, streamInitialized, error)) {
        return false;
    }

    payload.clear();
    stream.next_in = compressed.empty() ? nullptr : compressed.data();
    stream.avail_in = static_cast<uInt>(compressed.size());

    std::vector<CARD8> buffer(64 * 1024);
    do {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(buffer.size());
        const int rc = inflate(&stream, Z_SYNC_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            SetError(error, "failed to decompress RFB ZRLE payload");
            return false;
        }
        const std::size_t produced = buffer.size() - stream.avail_out;
        payload.insert(payload.end(), buffer.begin(), buffer.begin() + produced);
        if (rc == Z_STREAM_END) {
            inflateReset(&stream);
            break;
        }
        if (produced == 0 && stream.avail_in == 0) {
            break;
        }
    } while (stream.avail_in > 0 || stream.avail_out == 0);
    return true;
}

bool ReadBytesFromPayload(const std::vector<CARD8>& payload, std::size_t& offset, void *out, std::size_t length)
{
    if (offset + length > payload.size()) {
        return false;
    }
    std::memcpy(out, payload.data() + offset, length);
    offset += length;
    return true;
}

bool ReadZrlePixel(const rfbPixelFormat& format,
                   const std::vector<CARD8>& payload,
                   std::size_t& offset,
                   std::vector<CARD8>& pixel)
{
    const unsigned int compactBytes = ZrleBytesPerPixel(format);
    std::vector<CARD8> compact(compactBytes, 0);
    if (!ReadBytesFromPayload(payload, offset, compact.data(), compact.size())) {
        return false;
    }
    pixel = ExpandZrlePixel(format, compact);
    return pixel.size() == BytesPerPixel(format);
}

void WriteZrlePixel(std::vector<CARD8>& pixels,
                    unsigned int rectangleWidth,
                    unsigned int bytesPerPixel,
                    unsigned int x,
                    unsigned int y,
                    const std::vector<CARD8>& pixel)
{
    const std::size_t dst = (static_cast<std::size_t>(y) * rectangleWidth + x) * bytesPerPixel;
    std::copy(pixel.begin(), pixel.end(), pixels.begin() + dst);
}

bool ReadZrleRunLength(const std::vector<CARD8>& payload, std::size_t& offset, unsigned int& runLength)
{
    runLength = 1;
    for (;;) {
        CARD8 value = 0;
        if (!ReadBytesFromPayload(payload, offset, &value, sizeof(value))) {
            return false;
        }
        runLength += value;
        if (value != 255) {
            return true;
        }
    }
}

bool DecodeZrlePackedPaletteTile(const std::vector<CARD8>& payload,
                                 std::size_t& offset,
                                 const std::vector<std::vector<CARD8> >& palette,
                                 unsigned int tileWidth,
                                 unsigned int tileHeight,
                                 unsigned int tileX,
                                 unsigned int tileY,
                                 unsigned int rectangleWidth,
                                 unsigned int bytesPerPixel,
                                 std::vector<CARD8>& pixels,
                                 std::string *error)
{
    const unsigned int bitsPerPixel = palette.size() <= 2 ? 1 : (palette.size() <= 4 ? 2 : 4);
    const unsigned int pixelsPerByte = 8 / bitsPerPixel;
    const unsigned int rowBytes = (tileWidth + pixelsPerByte - 1) / pixelsPerByte;
    const CARD8 mask = static_cast<CARD8>((1u << bitsPerPixel) - 1u);
    for (unsigned int row = 0; row < tileHeight; ++row) {
        for (unsigned int byteIndex = 0; byteIndex < rowBytes; ++byteIndex) {
            CARD8 packed = 0;
            if (!ReadBytesFromPayload(payload, offset, &packed, sizeof(packed))) {
                SetError(error, "failed to read RFB ZRLE packed palette row");
                return false;
            }
            for (unsigned int packedPixel = 0; packedPixel < pixelsPerByte; ++packedPixel) {
                const unsigned int col = byteIndex * pixelsPerByte + packedPixel;
                if (col >= tileWidth) {
                    break;
                }
                const unsigned int shift = 8 - bitsPerPixel * (packedPixel + 1);
                const unsigned int index = (packed >> shift) & mask;
                if (index >= palette.size()) {
                    SetError(error, "RFB ZRLE palette index is outside palette bounds");
                    return false;
                }
                WriteZrlePixel(pixels, rectangleWidth, bytesPerPixel, tileX + col, tileY + row, palette[index]);
            }
        }
    }
    return true;
}

bool FillZrleRun(unsigned int& pixelIndex,
                 unsigned int runLength,
                 const std::vector<CARD8>& color,
                 unsigned int tileWidth,
                 unsigned int tileHeight,
                 unsigned int tileX,
                 unsigned int tileY,
                 unsigned int rectangleWidth,
                 unsigned int bytesPerPixel,
                 std::vector<CARD8>& pixels,
                 std::string *error)
{
    const unsigned int tilePixels = tileWidth * tileHeight;
    if (runLength == 0 || pixelIndex + runLength > tilePixels) {
        SetError(error, "RFB ZRLE RLE run exceeds tile bounds");
        return false;
    }
    for (unsigned int i = 0; i < runLength; ++i) {
        const unsigned int local = pixelIndex + i;
        const unsigned int x = tileX + (local % tileWidth);
        const unsigned int y = tileY + (local / tileWidth);
        WriteZrlePixel(pixels, rectangleWidth, bytesPerPixel, x, y, color);
    }
    pixelIndex += runLength;
    return true;
}

bool ReadTightCompactLength(RfbTransport& socket, CARD32& length, std::string *error)
{
    CARD8 b = 0;
    if (!socket.ReadExact(&b, sizeof(b))) {
        SetError(error, "failed to read RFB Tight compact length");
        return false;
    }
    length = b & 0x7f;
    if (b & 0x80) {
        if (!socket.ReadExact(&b, sizeof(b))) {
            SetError(error, "failed to read RFB Tight compact length");
            return false;
        }
        length |= static_cast<CARD32>(b & 0x7f) << 7;
        if (b & 0x80) {
            if (!socket.ReadExact(&b, sizeof(b))) {
                SetError(error, "failed to read RFB Tight compact length");
                return false;
            }
            length |= static_cast<CARD32>(b) << 14;
        }
    }
    return true;
}

bool EnsureTightInflateStream(z_stream& stream, bool& initialized, std::string *error)
{
    if (initialized) {
        return true;
    }
    std::memset(&stream, 0, sizeof(stream));
    const int rc = inflateInit(&stream);
    if (rc != Z_OK) {
        SetError(error, "failed to initialize RFB Tight inflater");
        return false;
    }
    initialized = true;
    return true;
}

bool ReadTightData(RfbTransport& socket,
                   z_stream& stream,
                   bool& streamInitialized,
                   unsigned int streamId,
                   std::size_t expectedSize,
                   bool uncompressed,
                   std::vector<CARD8>& payload,
                   std::string *error)
{
    payload.assign(expectedSize, 0);
    if (expectedSize == 0) {
        return true;
    }
    if (uncompressed || expectedSize < 12) {
        if (!socket.ReadExact(payload.data(), payload.size())) {
            SetError(error, "failed to read RFB Tight uncompressed payload");
            return false;
        }
        return true;
    }

    CARD32 compressedSize = 0;
    if (!ReadTightCompactLength(socket, compressedSize, error)) {
        return false;
    }
    std::vector<CARD8> compressed(compressedSize);
    if (compressedSize > 0 && !socket.ReadExact(compressed.data(), compressed.size())) {
        SetError(error, "failed to read RFB Tight compressed payload");
        return false;
    }
    if (streamId > 3) {
        SetError(error, "invalid RFB Tight zlib stream id");
        return false;
    }
    if (!EnsureTightInflateStream(stream, streamInitialized, error)) {
        return false;
    }
    stream.next_in = compressed.empty() ? nullptr : compressed.data();
    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_out = payload.data();
    stream.avail_out = static_cast<uInt>(payload.size());
    const int rc = inflate(&stream, Z_SYNC_FLUSH);
    if ((rc != Z_OK && rc != Z_STREAM_END) || stream.avail_out != 0) {
        SetError(error, "failed to decompress RFB Tight payload");
        return false;
    }
    return true;
}

unsigned int TightBytesPerPixel(const rfbPixelFormat& format)
{
    return ZrleBytesPerPixel(format);
}

std::vector<CARD8> PackRgbPixel(const rfbPixelFormat& format, CARD8 red, CARD8 green, CARD8 blue)
{
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    const CARD32 r = format.redMax == 255 ? red : static_cast<CARD32>((static_cast<unsigned int>(red) * format.redMax) / 255);
    const CARD32 g = format.greenMax == 255 ? green : static_cast<CARD32>((static_cast<unsigned int>(green) * format.greenMax) / 255);
    const CARD32 b = format.blueMax == 255 ? blue : static_cast<CARD32>((static_cast<unsigned int>(blue) * format.blueMax) / 255);
    const CARD32 value = ((r & format.redMax) << format.redShift) |
                         ((g & format.greenMax) << format.greenShift) |
                         ((b & format.blueMax) << format.blueShift);
    std::vector<CARD8> pixel(bytesPerPixel, 0);
    for (unsigned int i = 0; i < bytesPerPixel; ++i) {
        pixel[i] = static_cast<CARD8>((value >> (8 * i)) & 0xff);
    }
    return pixel;
}

bool DecodeTightJpegPayload(const rfbPixelFormat& format,
                            const std::vector<CARD8>& jpeg,
                            unsigned int width,
                            unsigned int height,
                            std::vector<CARD8>& pixels,
                            std::string *error)
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    std::memset(&cinfo, 0, sizeof(cinfo));
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, const_cast<unsigned char *>(jpeg.data()), static_cast<unsigned long>(jpeg.size()));
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        SetError(error, "failed to read RFB Tight JPEG header");
        return false;
    }
    cinfo.out_color_space = JCS_RGB;
    if (!jpeg_start_decompress(&cinfo)) {
        jpeg_destroy_decompress(&cinfo);
        SetError(error, "failed to start RFB Tight JPEG decompression");
        return false;
    }
    if (cinfo.output_width != width || cinfo.output_height != height || cinfo.output_components != 3) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        SetError(error, "RFB Tight JPEG dimensions do not match rectangle");
        return false;
    }

    const unsigned int bytesPerPixel = BytesPerPixel(format);
    pixels.assign(static_cast<std::size_t>(width) * height * bytesPerPixel, 0);
    std::vector<CARD8> row(static_cast<std::size_t>(width) * 3);
    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW rowPointer = row.data();
        const unsigned int y = cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, &rowPointer, 1);
        for (unsigned int x = 0; x < width; ++x) {
            const std::size_t src = static_cast<std::size_t>(x) * 3;
            const std::vector<CARD8> pixel = PackRgbPixel(format, row[src], row[src + 1], row[src + 2]);
            const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * bytesPerPixel;
            std::copy(pixel.begin(), pixel.end(), pixels.begin() + dst);
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return true;
}

bool DecodeTightGradientPayload(const rfbPixelFormat& format,
                                const std::vector<CARD8>& payload,
                                unsigned int width,
                                unsigned int height,
                                std::vector<CARD8>& pixels,
                                std::string *error)
{
    const unsigned int compactBytes = TightBytesPerPixel(format);
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    const std::size_t expected = static_cast<std::size_t>(width) * height * compactBytes;
    if (payload.size() != expected) {
        SetError(error, "invalid RFB Tight gradient payload size");
        return false;
    }
    pixels.assign(static_cast<std::size_t>(width) * height * bytesPerPixel, 0);
    std::vector<CARD8> previousRow(static_cast<std::size_t>(width) * compactBytes, 0);
    std::vector<CARD8> currentRow(static_cast<std::size_t>(width) * compactBytes, 0);
    std::size_t offset = 0;
    for (unsigned int y = 0; y < height; ++y) {
        std::fill(currentRow.begin(), currentRow.end(), 0);
        for (unsigned int x = 0; x < width; ++x) {
            std::vector<CARD8> compact(compactBytes, 0);
            for (unsigned int c = 0; c < compactBytes; ++c) {
                const int left = x == 0 ? 0 : currentRow[static_cast<std::size_t>(x - 1) * compactBytes + c];
                const int up = y == 0 ? 0 : previousRow[static_cast<std::size_t>(x) * compactBytes + c];
                const int upLeft = (x == 0 || y == 0) ? 0 : previousRow[static_cast<std::size_t>(x - 1) * compactBytes + c];
                const int predicted = std::max(0, std::min(255, left + up - upLeft));
                compact[c] = static_cast<CARD8>((predicted + payload[offset++]) & 0xff);
                currentRow[static_cast<std::size_t>(x) * compactBytes + c] = compact[c];
            }
            const std::vector<CARD8> pixel = ExpandZrlePixel(format, compact);
            if (pixel.size() != bytesPerPixel) {
                SetError(error, "failed to expand RFB Tight gradient pixel");
                return false;
            }
            const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * bytesPerPixel;
            std::copy(pixel.begin(), pixel.end(), pixels.begin() + dst);
        }
        previousRow.swap(currentRow);
    }
    return true;
}


bool ReadTightPixel(RfbTransport& socket, const rfbPixelFormat& format, std::vector<CARD8>& pixel, std::string *error)
{
    const unsigned int compactBytes = TightBytesPerPixel(format);
    std::vector<CARD8> compact(compactBytes, 0);
    if (!socket.ReadExact(compact.data(), compact.size())) {
        SetError(error, "failed to read RFB Tight pixel");
        return false;
    }
    pixel = ExpandZrlePixel(format, compact);
    return pixel.size() == BytesPerPixel(format);
}

bool DecodeTightCopyPayload(const rfbPixelFormat& format,
                            const std::vector<CARD8>& payload,
                            unsigned int width,
                            unsigned int height,
                            std::vector<CARD8>& pixels,
                            std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    const unsigned int compactBytes = TightBytesPerPixel(format);
    const std::size_t expected = static_cast<std::size_t>(width) * height * compactBytes;
    if (payload.size() != expected) {
        SetError(error, "invalid RFB Tight copy payload size");
        return false;
    }
    pixels.assign(static_cast<std::size_t>(width) * height * bytesPerPixel, 0);
    std::size_t offset = 0;
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            std::vector<CARD8> compact(compactBytes, 0);
            std::copy(payload.begin() + offset, payload.begin() + offset + compactBytes, compact.begin());
            offset += compactBytes;
            const std::vector<CARD8> pixel = ExpandZrlePixel(format, compact);
            if (pixel.size() != bytesPerPixel) {
                SetError(error, "failed to expand RFB Tight pixel");
                return false;
            }
            const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * bytesPerPixel;
            std::copy(pixel.begin(), pixel.end(), pixels.begin() + dst);
        }
    }
    return true;
}

bool DecodeTightPalettePayload(const rfbPixelFormat& format,
                               const std::vector<std::vector<CARD8> >& palette,
                               const std::vector<CARD8>& payload,
                               unsigned int width,
                               unsigned int height,
                               std::vector<CARD8>& pixels,
                               std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(format);
    pixels.assign(static_cast<std::size_t>(width) * height * bytesPerPixel, 0);
    if (palette.size() == 2) {
        const unsigned int rowBytes = (width + 7) / 8;
        if (payload.size() != static_cast<std::size_t>(rowBytes) * height) {
            SetError(error, "invalid RFB Tight binary palette payload size");
            return false;
        }
        std::size_t offset = 0;
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int byteIndex = 0; byteIndex < rowBytes; ++byteIndex) {
                const CARD8 packed = payload[offset++];
                for (unsigned int bit = 0; bit < 8; ++bit) {
                    const unsigned int x = byteIndex * 8 + bit;
                    if (x >= width) {
                        break;
                    }
                    const unsigned int index = (packed >> (7 - bit)) & 0x01;
                    const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * bytesPerPixel;
                    std::copy(palette[index].begin(), palette[index].end(), pixels.begin() + dst);
                }
            }
        }
        return true;
    }

    if (payload.size() != static_cast<std::size_t>(width) * height) {
        SetError(error, "invalid RFB Tight palette payload size");
        return false;
    }
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const unsigned int index = payload[static_cast<std::size_t>(y) * width + x];
            if (index >= palette.size()) {
                SetError(error, "RFB Tight palette index is outside palette bounds");
                return false;
            }
            const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * bytesPerPixel;
            std::copy(palette[index].begin(), palette[index].end(), pixels.begin() + dst);
        }
    }
    return true;
}

bool ReadTightRectPayload(RfbTransport& socket,
                          z_stream tightStreams[4],
                          bool tightStreamsInitialized[4],
                          ViewerSessionResult& result,
                          std::vector<CARD8>& framebuffer,
                          ViewerFramebufferRect& rectangle,
                          std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB Tight rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB Tight rectangle is outside framebuffer bounds");
        return false;
    }

    CARD8 control = 0;
    if (!socket.ReadExact(&control, sizeof(control))) {
        SetError(error, "failed to read RFB Tight control byte");
        return false;
    }
    for (unsigned int i = 0; i < 4; ++i) {
        if ((control & (1u << i)) && tightStreamsInitialized[i]) {
            inflateEnd(&tightStreams[i]);
            std::memset(&tightStreams[i], 0, sizeof(tightStreams[i]));
            tightStreamsInitialized[i] = false;
        }
    }
    const CARD8 subencoding = control >> 4;

    if (subencoding == rfbTightFill) {
        std::vector<CARD8> color;
        if (!ReadTightPixel(socket, result.format, color, error)) {
            return false;
        }
        rectangle.pixels = SolidPixelRect(color, rectangle.width, rectangle.height, bytesPerPixel);
        if (!EnsureFramebufferStorage(result, framebuffer, error)) {
            return false;
        }
        CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
        return true;
    }
    if (subencoding == rfbTightJpeg) {
        CARD32 jpegSize = 0;
        if (!ReadTightCompactLength(socket, jpegSize, error)) {
            return false;
        }
        std::vector<CARD8> jpeg(jpegSize);
        if (jpegSize > 0 && !socket.ReadExact(jpeg.data(), jpeg.size())) {
            SetError(error, "failed to read RFB Tight JPEG payload");
            return false;
        }
        if (!DecodeTightJpegPayload(result.format, jpeg, rectangle.width, rectangle.height, rectangle.pixels, error)) {
            return false;
        }
        if (!EnsureFramebufferStorage(result, framebuffer, error)) {
            return false;
        }
        CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
        return true;
    }
    if (subencoding > rfbTightNoZlib) {
        SetError(error, "unsupported RFB Tight subencoding");
        return false;
    }

    CARD8 filter = rfbTightFilterCopy;
    if (subencoding & rfbTightExplicitFilter) {
        if (!socket.ReadExact(&filter, sizeof(filter))) {
            SetError(error, "failed to read RFB Tight filter id");
            return false;
        }
    }
    std::vector<std::vector<CARD8> > palette;
    unsigned int bitsPerPixel = TightBytesPerPixel(result.format) * 8;
    if (filter == rfbTightFilterPalette) {
        CARD8 paletteSizeMinusOne = 0;
        if (!socket.ReadExact(&paletteSizeMinusOne, sizeof(paletteSizeMinusOne))) {
            SetError(error, "failed to read RFB Tight palette size");
            return false;
        }
        const unsigned int paletteSize = paletteSizeMinusOne + 1;
        if (paletteSize < 2) {
            SetError(error, "invalid RFB Tight palette size");
            return false;
        }
        for (unsigned int i = 0; i < paletteSize; ++i) {
            std::vector<CARD8> color;
            if (!ReadTightPixel(socket, result.format, color, error)) {
                return false;
            }
            palette.push_back(color);
        }
        bitsPerPixel = paletteSize == 2 ? 1 : 8;
    } else if (filter == rfbTightFilterGradient) {
        bitsPerPixel = TightBytesPerPixel(result.format) * 8;
    } else if (filter != rfbTightFilterCopy) {
        SetError(error, "unsupported RFB Tight filter");
        return false;
    }

    const unsigned int rowBytes = (rectangle.width * bitsPerPixel + 7) / 8;
    const std::size_t expected = static_cast<std::size_t>(rowBytes) * rectangle.height;
    std::vector<CARD8> payload;
    const bool uncompressed = subencoding == rfbTightNoZlib;
    const unsigned int streamId = subencoding & 0x03;
    if (!ReadTightData(socket, tightStreams[streamId], tightStreamsInitialized[streamId], streamId, expected, uncompressed, payload, error)) {
        return false;
    }
    if (filter == rfbTightFilterCopy) {
        if (!DecodeTightCopyPayload(result.format, payload, rectangle.width, rectangle.height, rectangle.pixels, error)) {
            return false;
        }
    } else if (filter == rfbTightFilterGradient) {
        if (!DecodeTightGradientPayload(result.format, payload, rectangle.width, rectangle.height, rectangle.pixels, error)) {
            return false;
        }
    } else if (!DecodeTightPalettePayload(result.format, palette, payload, rectangle.width, rectangle.height, rectangle.pixels, error)) {
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}

bool ReadZrleRectPayload(RfbTransport& socket,
                         z_stream& stream,
                         bool& streamInitialized,
                         ViewerSessionResult& result,
                         std::vector<CARD8>& framebuffer,
                         ViewerFramebufferRect& rectangle,
                         std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB ZRLE rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB ZRLE rectangle is outside framebuffer bounds");
        return false;
    }
    const std::size_t rawBytes = static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel;
    std::vector<CARD8> payload;
    if (!ReadZrleCompressedPayload(socket, stream, streamInitialized, payload, error)) {
        return false;
    }

    rectangle.pixels.assign(rawBytes, 0);
    std::size_t offset = 0;
    for (unsigned int tileY = 0; tileY < rectangle.height; tileY += rfbZRLETileHeight) {
        const unsigned int tileHeight = std::min<unsigned int>(rfbZRLETileHeight, rectangle.height - tileY);
        for (unsigned int tileX = 0; tileX < rectangle.width; tileX += rfbZRLETileWidth) {
            const unsigned int tileWidth = std::min<unsigned int>(rfbZRLETileWidth, rectangle.width - tileX);
            CARD8 subencoding = 0;
            if (!ReadBytesFromPayload(payload, offset, &subencoding, sizeof(subencoding))) {
                SetError(error, "failed to read RFB ZRLE tile subencoding");
                return false;
            }

            if (subencoding == 0) {
                for (unsigned int row = 0; row < tileHeight; ++row) {
                    for (unsigned int col = 0; col < tileWidth; ++col) {
                        std::vector<CARD8> pixel;
                        if (!ReadZrlePixel(result.format, payload, offset, pixel)) {
                            SetError(error, "failed to read RFB ZRLE raw tile pixel");
                            return false;
                        }
                        WriteZrlePixel(rectangle.pixels, rectangle.width, bytesPerPixel, tileX + col, tileY + row, pixel);
                    }
                }
                continue;
            }

            std::vector<std::vector<CARD8> > palette;
            if (subencoding >= 1 && subencoding <= 127) {
                const unsigned int paletteSize = subencoding;
                for (unsigned int i = 0; i < paletteSize; ++i) {
                    std::vector<CARD8> pixel;
                    if (!ReadZrlePixel(result.format, payload, offset, pixel)) {
                        SetError(error, "failed to read RFB ZRLE palette pixel");
                        return false;
                    }
                    palette.push_back(pixel);
                }
                if (paletteSize == 1) {
                    for (unsigned int row = 0; row < tileHeight; ++row) {
                        for (unsigned int col = 0; col < tileWidth; ++col) {
                            WriteZrlePixel(rectangle.pixels, rectangle.width, bytesPerPixel, tileX + col, tileY + row, palette[0]);
                        }
                    }
                    continue;
                }
                if (!DecodeZrlePackedPaletteTile(payload, offset, palette, tileWidth, tileHeight, tileX, tileY,
                                                 rectangle.width, bytesPerPixel, rectangle.pixels, error)) {
                    return false;
                }
                continue;
            }

            if (subencoding == 128) {
                unsigned int pixelIndex = 0;
                while (pixelIndex < tileWidth * tileHeight) {
                    std::vector<CARD8> color;
                    unsigned int runLength = 0;
                    if (!ReadZrlePixel(result.format, payload, offset, color) || !ReadZrleRunLength(payload, offset, runLength)) {
                        SetError(error, "failed to read RFB ZRLE plain RLE run");
                        return false;
                    }
                    if (!FillZrleRun(pixelIndex, runLength, color, tileWidth, tileHeight, tileX, tileY,
                                     rectangle.width, bytesPerPixel, rectangle.pixels, error)) {
                        return false;
                    }
                }
                continue;
            }

            if (subencoding >= 130) {
                const unsigned int paletteSize = subencoding - 128;
                if (paletteSize > 127) {
                    SetError(error, "unsupported RFB ZRLE palette RLE size");
                    return false;
                }
                for (unsigned int i = 0; i < paletteSize; ++i) {
                    std::vector<CARD8> pixel;
                    if (!ReadZrlePixel(result.format, payload, offset, pixel)) {
                        SetError(error, "failed to read RFB ZRLE RLE palette pixel");
                        return false;
                    }
                    palette.push_back(pixel);
                }
                unsigned int pixelIndex = 0;
                while (pixelIndex < tileWidth * tileHeight) {
                    CARD8 packedIndex = 0;
                    if (!ReadBytesFromPayload(payload, offset, &packedIndex, sizeof(packedIndex))) {
                        SetError(error, "failed to read RFB ZRLE palette RLE index");
                        return false;
                    }
                    const bool hasRunLength = (packedIndex & 0x80) != 0;
                    const unsigned int paletteIndex = packedIndex & 0x7f;
                    if (paletteIndex >= palette.size()) {
                        SetError(error, "RFB ZRLE palette RLE index is outside palette bounds");
                        return false;
                    }
                    unsigned int runLength = 1;
                    if (hasRunLength && !ReadZrleRunLength(payload, offset, runLength)) {
                        SetError(error, "failed to read RFB ZRLE palette RLE run length");
                        return false;
                    }
                    if (!FillZrleRun(pixelIndex, runLength, palette[paletteIndex], tileWidth, tileHeight, tileX, tileY,
                                     rectangle.width, bytesPerPixel, rectangle.pixels, error)) {
                        return false;
                    }
                }
                continue;
            }

            SetError(error, "unsupported RFB ZRLE tile subencoding");
            return false;
        }
    }
    if (offset != payload.size()) {
        SetError(error, "RFB ZRLE payload has trailing bytes");
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}


bool ReadRawRectPayload(RfbTransport& socket,
                        ViewerSessionResult& result,
                        std::vector<CARD8>& framebuffer,
                        ViewerFramebufferRect& rectangle,
                        std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB raw rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB raw rectangle is outside framebuffer bounds");
        return false;
    }
    rectangle.pixels.resize(static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel);
    if (!socket.ReadExact(rectangle.pixels.data(), rectangle.pixels.size())) {
        SetError(error, "failed to read RFB raw framebuffer update pixels");
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}


bool ReadZlibRectPayload(RfbTransport& socket,
                         ViewerSessionResult& result,
                         std::vector<CARD8>& framebuffer,
                         ViewerFramebufferRect& rectangle,
                         std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB zlib rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB zlib rectangle is outside framebuffer bounds");
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel;
    if (!ReadCompressedPayload(socket, expected, rectangle.pixels, error)) {
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}

bool ReadRreRectPayload(RfbTransport& socket,
                        ViewerSessionResult& result,
                        std::vector<CARD8>& framebuffer,
                        ViewerFramebufferRect& rectangle,
                        bool compact,
                        std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB RRE rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB RRE rectangle is outside framebuffer bounds");
        return false;
    }
    rfbRREHeader header;
    if (!socket.ReadExact(&header, sz_rfbRREHeader)) {
        SetError(error, "failed to read RFB RRE header");
        return false;
    }
    const CARD32 subrects = Swap32IfLE(header.nSubrects);
    std::vector<CARD8> background;
    if (!ReadPixel(socket, bytesPerPixel, background, error)) {
        return false;
    }
    rectangle.pixels = SolidPixelRect(background, rectangle.width, rectangle.height, bytesPerPixel);
    for (CARD32 i = 0; i < subrects; ++i) {
        std::vector<CARD8> color;
        if (!ReadPixel(socket, bytesPerPixel, color, error)) {
            return false;
        }
        unsigned int sx = 0;
        unsigned int sy = 0;
        unsigned int sw = 0;
        unsigned int sh = 0;
        if (compact) {
            rfbCoRRERectangle subrect;
            if (!socket.ReadExact(&subrect, sz_rfbCoRRERectangle)) {
                SetError(error, "failed to read RFB CoRRE subrectangle");
                return false;
            }
            sx = subrect.x;
            sy = subrect.y;
            sw = subrect.w;
            sh = subrect.h;
        } else {
            rfbRectangle subrect;
            if (!socket.ReadExact(&subrect, sz_rfbRectangle)) {
                SetError(error, "failed to read RFB RRE subrectangle");
                return false;
            }
            sx = Swap16IfLE(subrect.x);
            sy = Swap16IfLE(subrect.y);
            sw = Swap16IfLE(subrect.w);
            sh = Swap16IfLE(subrect.h);
        }
        if (sw == 0 || sh == 0 || sx + sw > rectangle.width || sy + sh > rectangle.height) {
            SetError(error, "RFB RRE subrectangle is outside rectangle bounds");
            return false;
        }
        for (unsigned int row = 0; row < sh; ++row) {
            for (unsigned int col = 0; col < sw; ++col) {
                const std::size_t dst = (static_cast<std::size_t>(sy + row) * rectangle.width + sx + col) * bytesPerPixel;
                std::copy(color.begin(), color.end(), rectangle.pixels.begin() + dst);
            }
        }
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}

bool ReadHextileRectPayload(RfbTransport& socket,
                            ViewerSessionResult& result,
                            std::vector<CARD8>& framebuffer,
                            ViewerFramebufferRect& rectangle,
                            std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB Hextile rectangle dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height) {
        SetError(error, "RFB Hextile rectangle is outside framebuffer bounds");
        return false;
    }
    rectangle.pixels.assign(static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel, 0);
    std::vector<CARD8> background(bytesPerPixel, 0);
    std::vector<CARD8> foreground(bytesPerPixel, 0);

    for (unsigned int tileY = 0; tileY < rectangle.height; tileY += 16) {
        const unsigned int tileHeight = std::min(16u, rectangle.height - tileY);
        for (unsigned int tileX = 0; tileX < rectangle.width; tileX += 16) {
            const unsigned int tileWidth = std::min(16u, rectangle.width - tileX);
            CARD8 subencoding = 0;
            if (!socket.ReadExact(&subencoding, sizeof(subencoding))) {
                SetError(error, "failed to read RFB Hextile subencoding");
                return false;
            }
            if (subencoding & (rfbHextileZlibHex | rfbHextileZlibMono)) {
                SetError(error, "unsupported RFB ZlibHex encoded tile variant");
                return false;
            }
            if (subencoding & (rfbHextileRaw | rfbHextileZlibRaw)) {
                const std::size_t rawSize = static_cast<std::size_t>(tileWidth) * tileHeight * bytesPerPixel;
                std::vector<CARD8> raw;
                if (subencoding & rfbHextileZlibRaw) {
                    CARD16 compressedLength = 0;
                    if (!socket.ReadExact(&compressedLength, sizeof(compressedLength))) {
                        SetError(error, "failed to read RFB ZlibHex raw tile length");
                        return false;
                    }
                    compressedLength = Swap16IfLE(compressedLength);
                    std::vector<CARD8> compressed(compressedLength);
                    if (compressedLength > 0 && !socket.ReadExact(compressed.data(), compressed.size())) {
                        SetError(error, "failed to read RFB ZlibHex raw tile payload");
                        return false;
                    }
                    raw.assign(rawSize, 0);
                    uLongf outputSize = static_cast<uLongf>(raw.size());
                    const int rc = uncompress(raw.data(), &outputSize, compressed.data(), static_cast<uLong>(compressed.size()));
                    if (rc != Z_OK || outputSize != raw.size()) {
                        SetError(error, "failed to decompress RFB ZlibHex raw tile");
                        return false;
                    }
                } else {
                    raw.assign(rawSize, 0);
                    if (!socket.ReadExact(raw.data(), raw.size())) {
                        SetError(error, "failed to read RFB Hextile raw tile");
                        return false;
                    }
                }
                for (unsigned int row = 0; row < tileHeight; ++row) {
                    const std::size_t src = static_cast<std::size_t>(row) * tileWidth * bytesPerPixel;
                    const std::size_t dst = (static_cast<std::size_t>(tileY + row) * rectangle.width + tileX) * bytesPerPixel;
                    std::copy(raw.begin() + src,
                              raw.begin() + src + static_cast<std::size_t>(tileWidth) * bytesPerPixel,
                              rectangle.pixels.begin() + dst);
                }
                continue;
            }
            if ((subencoding & rfbHextileBackgroundSpecified) && !ReadPixel(socket, bytesPerPixel, background, error)) {
                return false;
            }
            if (subencoding & rfbHextileForegroundSpecified) {
                if (!ReadPixel(socket, bytesPerPixel, foreground, error)) {
                    return false;
                }
            }
            for (unsigned int row = 0; row < tileHeight; ++row) {
                for (unsigned int col = 0; col < tileWidth; ++col) {
                    const std::size_t dst = (static_cast<std::size_t>(tileY + row) * rectangle.width + tileX + col) * bytesPerPixel;
                    std::copy(background.begin(), background.end(), rectangle.pixels.begin() + dst);
                }
            }
            if (!(subencoding & rfbHextileAnySubrects)) {
                continue;
            }
            CARD8 subrects = 0;
            if (!socket.ReadExact(&subrects, sizeof(subrects))) {
                SetError(error, "failed to read RFB Hextile subrectangle count");
                return false;
            }
            for (CARD8 i = 0; i < subrects; ++i) {
                std::vector<CARD8> color = foreground;
                if (subencoding & rfbHextileSubrectsColoured) {
                    if (!ReadPixel(socket, bytesPerPixel, color, error)) {
                        return false;
                    }
                }
                CARD8 xy = 0;
                CARD8 wh = 0;
                if (!socket.ReadExact(&xy, sizeof(xy)) || !socket.ReadExact(&wh, sizeof(wh))) {
                    SetError(error, "failed to read RFB Hextile subrectangle geometry");
                    return false;
                }
                const unsigned int sx = rfbHextileExtractX(xy);
                const unsigned int sy = rfbHextileExtractY(xy);
                const unsigned int sw = rfbHextileExtractW(wh);
                const unsigned int sh = rfbHextileExtractH(wh);
                if (sx + sw > tileWidth || sy + sh > tileHeight) {
                    SetError(error, "RFB Hextile subrectangle is outside tile bounds");
                    return false;
                }
                for (unsigned int row = 0; row < sh; ++row) {
                    for (unsigned int col = 0; col < sw; ++col) {
                        const std::size_t dst = (static_cast<std::size_t>(tileY + sy + row) * rectangle.width + tileX + sx + col) * bytesPerPixel;
                        std::copy(color.begin(), color.end(), rectangle.pixels.begin() + dst);
                    }
                }
            }
        }
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }
    CopyRectToFramebuffer(result, framebuffer, rectangle.x, rectangle.y, rectangle.width, rectangle.height, rectangle.pixels);
    return true;
}

bool ApplyCopyRectPayload(RfbTransport& socket,
                          ViewerSessionResult& result,
                          std::vector<CARD8>& framebuffer,
                          ViewerFramebufferRect& rectangle,
                          std::string *error)
{
    rfbCopyRect copyRect;
    if (!socket.ReadExact(&copyRect, sz_rfbCopyRect)) {
        SetError(error, "failed to read RFB CopyRect payload");
        return false;
    }
    rectangle.sourceX = Swap16IfLE(copyRect.srcX);
    rectangle.sourceY = Swap16IfLE(copyRect.srcY);
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    if (bytesPerPixel == 0 || rectangle.width == 0 || rectangle.height == 0) {
        SetError(error, "invalid RFB CopyRect dimensions");
        return false;
    }
    if (rectangle.x + rectangle.width > result.width || rectangle.y + rectangle.height > result.height ||
        rectangle.sourceX + rectangle.width > result.width || rectangle.sourceY + rectangle.height > result.height) {
        SetError(error, "RFB CopyRect rectangle is outside framebuffer bounds");
        return false;
    }
    if (!EnsureFramebufferStorage(result, framebuffer, error)) {
        return false;
    }

    std::vector<CARD8> copy(static_cast<std::size_t>(rectangle.width) * rectangle.height * bytesPerPixel);
    for (unsigned int row = 0; row < rectangle.height; ++row) {
        const std::size_t src = (static_cast<std::size_t>(rectangle.sourceY + row) * result.width + rectangle.sourceX) * bytesPerPixel;
        const std::size_t dst = static_cast<std::size_t>(row) * rectangle.width * bytesPerPixel;
        std::copy(framebuffer.begin() + src,
                  framebuffer.begin() + src + static_cast<std::size_t>(rectangle.width) * bytesPerPixel,
                  copy.begin() + dst);
    }
    for (unsigned int row = 0; row < rectangle.height; ++row) {
        const std::size_t src = static_cast<std::size_t>(row) * rectangle.width * bytesPerPixel;
        const std::size_t dst = (static_cast<std::size_t>(rectangle.y + row) * result.width + rectangle.x) * bytesPerPixel;
        std::copy(copy.begin() + src,
                  copy.begin() + src + static_cast<std::size_t>(rectangle.width) * bytesPerPixel,
                  framebuffer.begin() + dst);
    }
    return true;
}

bool PublishCompositedFramebuffer(ViewerSessionResult& result,
                                  const std::vector<CARD8>& framebuffer,
                                  std::string *error)
{
    const unsigned int bytesPerPixel = BytesPerPixel(result.format);
    const std::size_t expected = static_cast<std::size_t>(result.width) * result.height * bytesPerPixel;
    if (bytesPerPixel == 0 || framebuffer.size() != expected) {
        SetError(error, "invalid composed RFB framebuffer size");
        return false;
    }
    result.update = ViewerFramebufferUpdate();
    result.update.received = true;
    result.update.x = 0;
    result.update.y = 0;
    result.update.width = result.width;
    result.update.height = result.height;
    result.update.encoding = rfbEncodingRaw;
    result.update.pixels = framebuffer;
    return true;
}


bool ReadRichCursorPayload(RfbTransport& socket,
                           ViewerSessionResult& result,
                           ViewerFramebufferRect& rectangle,
                           std::string *error)
{
    const std::size_t pixelBytes = static_cast<std::size_t>(rectangle.width) * rectangle.height * 4;
    const std::size_t maskBytes = static_cast<std::size_t>((rectangle.width + 7) / 8) * rectangle.height;
    rectangle.pixels.assign(pixelBytes, 0);
    if (pixelBytes > 0 && !socket.ReadExact(rectangle.pixels.data(), rectangle.pixels.size())) {
        SetError(error, "failed to read RFB RichCursor pixels");
        return false;
    }
    std::vector<CARD8> mask(maskBytes);
    if (maskBytes > 0 && !socket.ReadExact(mask.data(), mask.size())) {
        SetError(error, "failed to read RFB RichCursor mask");
        return false;
    }
    result.cursorShape.received = true;
    result.cursorShape.hotspotX = rectangle.x;
    result.cursorShape.hotspotY = rectangle.y;
    result.cursorShape.width = rectangle.width;
    result.cursorShape.height = rectangle.height;
    result.cursorShape.encoding = rectangle.encoding;
    result.cursorShape.pixels = rectangle.pixels;
    return true;
}

bool ReadXCursorPayload(RfbTransport& socket,
                        ViewerSessionResult& result,
                        ViewerFramebufferRect& rectangle,
                        std::string *error)
{
    if (rectangle.width == 0 || rectangle.height == 0) {
        result.cursorShape = ViewerCursorShape();
        result.cursorShape.received = true;
        result.cursorShape.encoding = rectangle.encoding;
        return true;
    }
    rfbXCursorColors colors;
    if (!socket.ReadExact(&colors, sz_rfbXCursorColors)) {
        SetError(error, "failed to read RFB XCursor colors");
        return false;
    }
    const std::size_t bitmapBytes = static_cast<std::size_t>((rectangle.width + 7) / 8) * rectangle.height;
    std::vector<CARD8> data(bitmapBytes);
    std::vector<CARD8> mask(bitmapBytes);
    if ((bitmapBytes > 0 && !socket.ReadExact(data.data(), data.size())) ||
        (bitmapBytes > 0 && !socket.ReadExact(mask.data(), mask.size()))) {
        SetError(error, "failed to read RFB XCursor bitmaps");
        return false;
    }
    result.cursorShape.received = true;
    result.cursorShape.hotspotX = rectangle.x;
    result.cursorShape.hotspotY = rectangle.y;
    result.cursorShape.width = rectangle.width;
    result.cursorShape.height = rectangle.height;
    result.cursorShape.encoding = rectangle.encoding;
    result.cursorShape.pixels.clear();
    return true;
}

bool ReadFramebufferUpdate(RfbTransport& socket,
                           z_stream& zrleStream,
                           bool& zrleStreamInitialized,
                           z_stream tightStreams[4],
                           bool tightStreamsInitialized[4],
                           ViewerSessionResult& result,
                           std::vector<CARD8>& framebuffer,
                           std::string *error)
{
    rfbFramebufferUpdateMsg update;
    for (unsigned int messages = 0; messages < 64; ++messages) {
        CARD8 type = 0;
        if (!socket.ReadExact(&type, sizeof(type))) {
            SetError(error, "failed to read RFB server message type");
            return false;
        }
        if (type == rfbBell) {
            result.bellCount += 1;
            continue;
        }
        if (type == rfbServerCutText) {
            rfbServerCutTextMsg cutText;
            std::memset(&cutText, 0, sizeof(cutText));
            cutText.type = type;
            if (!socket.ReadExact(reinterpret_cast<char *>(&cutText) + sizeof(type), sz_rfbServerCutTextMsg - sizeof(type))) {
                SetError(error, "failed to read RFB ServerCutText header");
                return false;
            }
            CARD32 length = Swap32IfLE(cutText.length);
            if (IsExtendedClipboardWireLength(cutText.length)) {
                length = ExtendedClipboardPayloadLength(cutText.length);
                std::vector<CARD8> payload(length);
                if (length > 0 && !socket.ReadExact(payload.data(), payload.size())) {
                    SetError(error, "failed to read RFB extended clipboard payload");
                    return false;
                }
                ExtendedClipboardPayload extended;
                if (!DecodeExtendedClipboardPayload(payload, extended)) {
                    SetError(error, "failed to decode RFB extended clipboard payload");
                    return false;
                }
                result.extendedClipboardReceived = true;
                if (extended.textPresent) {
                    result.serverCutText = extended.text;
                }
            } else {
                result.serverCutText.assign(length, '\0');
                if (length > 0 && !socket.ReadExact(&result.serverCutText[0], length)) {
                    SetError(error, "failed to read RFB ServerCutText payload");
                    return false;
                }
            }
            continue;
        }
        if (type != rfbFramebufferUpdate) {
            SetError(error, "unexpected RFB server message type");
            return false;
        }
        std::memset(&update, 0, sizeof(update));
        update.type = type;
        if (!socket.ReadExact(reinterpret_cast<char *>(&update) + sizeof(type), sz_rfbFramebufferUpdateMsg - sizeof(type))) {
            SetError(error, "failed to read RFB framebuffer update header");
            return false;
        }
        break;
    }
    if (update.type != rfbFramebufferUpdate) {
        SetError(error, "RFB framebuffer update was not received");
        return false;
    }

    result.rectangles.clear();
    const CARD16 rects = Swap16IfLE(update.nRects);
    if (rects == 0) {
        result.update = ViewerFramebufferUpdate();
        result.update.received = true;
        return true;
    }

    bool acceptedUpdate = false;
    for (CARD16 i = 0; i < rects; ++i) {
        rfbFramebufferUpdateRectHeader rect;
        if (!socket.ReadExact(&rect, sz_rfbFramebufferUpdateRectHeader)) {
            SetError(error, "failed to read RFB framebuffer update rectangle");
            return false;
        }

        ViewerFramebufferRect rectangle;
        rectangle.x = Swap16IfLE(rect.r.x);
        rectangle.y = Swap16IfLE(rect.r.y);
        rectangle.width = Swap16IfLE(rect.r.w);
        rectangle.height = Swap16IfLE(rect.r.h);
        rectangle.encoding = Swap32IfLE(rect.encoding);

        if (rectangle.encoding == rfbEncodingLastRect) {
            acceptedUpdate = true;
            result.rectangles.push_back(rectangle);
            break;
        } else if (rectangle.encoding == rfbEncodingPointerPos) {
            result.pointerPositionReceived = true;
            result.pointerX = rectangle.x;
            result.pointerY = rectangle.y;
        } else if (rectangle.encoding == rfbEncodingRichCursor) {
            if (!ReadRichCursorPayload(socket, result, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingXCursor) {
            if (!ReadXCursorPayload(socket, result, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingRaw) {
            if (!ReadRawRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingCopyRect) {
            if (!ApplyCopyRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingRRE) {
            if (!ReadRreRectPayload(socket, result, framebuffer, rectangle, false, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingCoRRE) {
            if (!ReadRreRectPayload(socket, result, framebuffer, rectangle, true, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingHextile) {
            if (!ReadHextileRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingZlib) {
            if (!ReadZlibRectPayload(socket, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingZRLE) {
            if (!ReadZrleRectPayload(socket, zrleStream, zrleStreamInitialized, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingTight) {
            if (!ReadTightRectPayload(socket, tightStreams, tightStreamsInitialized, result, framebuffer, rectangle, error)) {
                return false;
            }
        } else if (rectangle.encoding == rfbEncodingNewFBSize) {
            result.width = rectangle.width;
            result.height = rectangle.height;
            framebuffer.clear();
            acceptedUpdate = true;
        } else {
            SetUnsupportedEncodingError(error, rectangle.encoding);
            return false;
        }
        if (rectangle.encoding != rfbEncodingPointerPos &&
            rectangle.encoding != rfbEncodingRichCursor &&
            rectangle.encoding != rfbEncodingXCursor) {
            acceptedUpdate = true;
        }
        result.rectangles.push_back(rectangle);
    }

    if (!framebuffer.empty()) {
        return PublishCompositedFramebuffer(result, framebuffer, error);
    }
    result.update = ViewerFramebufferUpdate();
    result.update.received = acceptedUpdate;
    return true;
}


} // namespace

ViewerCursorShape::ViewerCursorShape()
    : received(false),
      hotspotX(0),
      hotspotY(0),
      width(0),
      height(0),
      encoding(0),
      pixels()
{
}

ViewerFramebufferRect::ViewerFramebufferRect()
    : x(0),
      y(0),
      width(0),
      height(0),
      sourceX(0),
      sourceY(0),
      encoding(0),
      pixels()
{
}

ViewerFramebufferUpdate::ViewerFramebufferUpdate()
    : received(false),
      x(0),
      y(0),
      width(0),
      height(0),
      sourceX(0),
      sourceY(0),
      encoding(0),
      pixels()
{
}

ViewerSessionResult::ViewerSessionResult()
    : width(0),
      height(0),
      format(),
      desktopName(),
      serverCutText(),
      extendedClipboardReceived(false),
      bellCount(0),
      pointerPositionReceived(false),
      pointerX(0),
      pointerY(0),
      cursorShape(),
      update(),
      rectangles()
{
    std::memset(&format, 0, sizeof(format));
}

bool ViewerSession::RunHandshake(const ViewerConfig& config, ViewerSessionResult& result, std::string *error) const
{
    std::string validationError;
    if (!config.Validate(&validationError)) {
        SetError(error, validationError);
        return false;
    }

    TcpSocket socket;
    if (!TcpSocket::Connect(config.Host(), config.Port(), socket)) {
        SetError(error, "failed to connect to RFB server");
        return false;
    }
    TcpRfbTransport transport(socket);
    std::unique_ptr<RfbTransport> tlsTransport;
    result = ViewerSessionResult();
    return RunHandshakeOnTransport(transport, config, result, nullptr, error);
}

bool ViewerSession::RequestOneFramebufferUpdate(const ViewerConfig& config, ViewerSessionResult& result, std::string *error) const
{
    PersistentViewerSession session;
    if (!session.Connect(config, result, error)) {
        return false;
    }
    return session.RequestFramebufferUpdate(false, result, error);
}

PersistentViewerSession::PersistentViewerSession()
    : socket_(),
      transport_(),
      state_(),
      config_(),
      framebuffer_(),
      zrleStream_(),
      zrleStreamInitialized_(false),
      tightStreams_(),
      tightStreamsInitialized_()
{
    std::memset(&zrleStream_, 0, sizeof(zrleStream_));
    std::memset(tightStreams_, 0, sizeof(tightStreams_));
    std::memset(tightStreamsInitialized_, 0, sizeof(tightStreamsInitialized_));
}

PersistentViewerSession::~PersistentViewerSession()
{
    Disconnect();
}

bool PersistentViewerSession::Connect(const ViewerConfig& config, ViewerSessionResult& result, std::string *error)
{
    std::string validationError;
    if (!config.Validate(&validationError)) {
        SetError(error, validationError);
        return false;
    }

    Disconnect();
    if (!TcpSocket::Connect(config.Host(), config.Port(), socket_)) {
        SetError(error, "failed to connect to RFB server");
        return false;
    }
    if (!socket_.SetTimeoutMs(config.SocketTimeoutMs())) {
        SetError(error, "failed to configure RFB socket timeout");
        Disconnect();
        return false;
    }
    transport_.reset(new TcpRfbTransport(socket_));
    state_ = ViewerSessionResult();
    config_ = config;
    if (!RunHandshakeOnTransport(*transport_, config, state_, &transport_, error)) {
        Disconnect();
        return false;
    }
    result = state_;
    return true;
}

bool PersistentViewerSession::Connected() const
{
    return socket_.Valid();
}

void PersistentViewerSession::Disconnect()
{
    transport_.reset();
    socket_.Close();
    if (zrleStreamInitialized_) {
        inflateEnd(&zrleStream_);
        std::memset(&zrleStream_, 0, sizeof(zrleStream_));
        zrleStreamInitialized_ = false;
    }
    for (unsigned int i = 0; i < 4; ++i) {
        if (tightStreamsInitialized_[i]) {
            inflateEnd(&tightStreams_[i]);
            std::memset(&tightStreams_[i], 0, sizeof(tightStreams_[i]));
            tightStreamsInitialized_[i] = false;
        }
    }
    state_ = ViewerSessionResult();
    framebuffer_.clear();
}

bool PersistentViewerSession::RequestFramebufferUpdate(bool incremental, ViewerSessionResult& result, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }

    uvnc::winvnc::portable::FramebufferUpdateRequest request;
    request.incremental = incremental;
    request.x = 0;
    request.y = 0;
    request.width = state_.width;
    request.height = state_.height;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    if (!transport_->WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg)) {
        SetError(error, "failed to write RFB framebuffer update request");
        Disconnect();
        return false;
    }
    state_.update = ViewerFramebufferUpdate();
    bool gotUpdate = false;
    for (unsigned int attempts = 0; attempts < 16 && !gotUpdate; ++attempts) {
        if (!ReadFramebufferUpdate(*transport_, zrleStream_, zrleStreamInitialized_, tightStreams_, tightStreamsInitialized_, state_, framebuffer_, error)) {
            Disconnect();
            return false;
        }
        gotUpdate = state_.update.received;
    }
    if (!gotUpdate) {
        SetError(error, "RFB framebuffer update did not contain a drawable update");
        Disconnect();
        return false;
    }
    result = state_;
    return true;
}

bool PersistentViewerSession::SendKeyEvent(CARD32 keysym, bool down, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const KeyEvent event{down, keysym};
    const rfbKeyEventMsg wire = EncodeKeyEvent(event);
    if (!transport_->WriteAll(&wire, sz_rfbKeyEventMsg)) {
        SetError(error, "failed to write RFB key event");
        Disconnect();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool PersistentViewerSession::SendPointerEvent(CARD8 buttonMask, unsigned int x, unsigned int y, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const PointerEvent event{buttonMask, x, y};
    const rfbPointerEventMsg wire = EncodePointerEvent(event);
    if (!transport_->WriteAll(&wire, sz_rfbPointerEventMsg)) {
        SetError(error, "failed to write RFB pointer event");
        Disconnect();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}


bool PersistentViewerSession::SendClientCutText(const std::string& text, std::string *error)
{
    if (!Connected()) {
        SetError(error, "RFB viewer session is not connected");
        return false;
    }
    const std::vector<CARD8> bytes = EncodeClientCutText(text);
    if (!transport_->WriteAll(bytes.data(), bytes.size())) {
        SetError(error, "failed to write RFB client cut text");
        Disconnect();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool PersistentViewerSession::RequestRemoteDirectory(const std::string& path, std::vector<ViewerFileTransferEntry>& entries, std::string *error)
{
    if (!Connected()) {
        SetError(error, "viewer is not connected");
        return false;
    }
    return RequestViewerDirectoryListing(socket_, path, entries, error);
}

bool PersistentViewerSession::RequestRemoteDrives(std::vector<ViewerFileTransferEntry>& entries, std::string *error)
{
    if (!Connected()) {
        SetError(error, "viewer is not connected");
        return false;
    }
    return RequestViewerDrivesList(socket_, entries, error);
}

bool PersistentViewerSession::DownloadRemoteFile(const std::string& path, ViewerFileDownload& download, std::string *error)
{
    if (!Connected()) {
        SetError(error, "viewer is not connected");
        return false;
    }
    return RequestViewerFileDownload(socket_, path, download, error);
}

bool PersistentViewerSession::RequestRemoteFileChecksums(const std::string& path, std::vector<std::string>& checksums, std::string *error)
{
    if (!Connected()) {
        SetError(error, "viewer is not connected");
        return false;
    }
    return RequestViewerFileChecksums(socket_, path, checksums, error);
}

} // namespace portable
} // namespace vncviewer
} // namespace uvnc
