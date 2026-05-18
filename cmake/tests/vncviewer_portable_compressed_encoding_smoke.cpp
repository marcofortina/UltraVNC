// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"
#include "vncPortableTcp.h"
#include "vncPortableViewerConfig.h"
#include "vncPortableViewerSession.h"

#include <zlib.h>

#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace uvnc::vncviewer::portable;
using namespace uvnc::winvnc::portable;

namespace {

rfbPixelFormat TestPixelFormat()
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = 32;
    format.depth = 24;
    format.trueColour = 1;
    format.redMax = Swap16IfLE(static_cast<CARD16>(255));
    format.greenMax = Swap16IfLE(static_cast<CARD16>(255));
    format.blueMax = Swap16IfLE(static_cast<CARD16>(255));
    format.redShift = 16;
    format.greenShift = 8;
    return format;
}

std::vector<CARD8> Pixel(CARD8 value)
{
    return std::vector<CARD8>{value, static_cast<CARD8>(value + 1), static_cast<CARD8>(value + 2), 0xff};
}

std::vector<CARD8> CompactPixel(CARD8 value)
{
    return std::vector<CARD8>{value, static_cast<CARD8>(value + 1), static_cast<CARD8>(value + 2)};
}

std::vector<CARD8> Solid(CARD8 value, unsigned int width, unsigned int height)
{
    const std::vector<CARD8> pixel = Pixel(value);
    std::vector<CARD8> bytes(static_cast<std::size_t>(width) * height * pixel.size());
    for (std::size_t offset = 0; offset < bytes.size(); offset += pixel.size()) {
        std::copy(pixel.begin(), pixel.end(), bytes.begin() + offset);
    }
    return bytes;
}

bool WriteServerInit(TcpSocket& client)
{
    const std::string name = "compressed-encoding-test";
    rfbServerInitMsg init;
    std::memset(&init, 0, sizeof(init));
    init.framebufferWidth = Swap16IfLE(static_cast<CARD16>(8));
    init.framebufferHeight = Swap16IfLE(static_cast<CARD16>(8));
    init.format = TestPixelFormat();
    init.nameLength = Swap32IfLE(static_cast<CARD32>(name.size()));
    return client.WriteAll(&init, sz_rfbServerInitMsg) && client.WriteAll(name.data(), name.size());
}

bool RunHandshake(TcpSocket& client)
{
    const std::string version = ProtocolVersion38();
    if (!client.WriteAll(version.data(), version.size())) return false;
    char clientVersion[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(clientVersion, sizeof(clientVersion))) return false;
    CARD8 security[2] = {1, rfbNoAuth};
    if (!client.WriteAll(security, sizeof(security))) return false;
    CARD8 selected = 0;
    if (!client.ReadExact(&selected, sizeof(selected)) || selected != rfbNoAuth) return false;
    CARD32 ok = Swap32IfLE(static_cast<CARD32>(rfbVncAuthOK));
    if (!client.WriteAll(&ok, sizeof(ok))) return false;
    rfbClientInitMsg clientInit;
    if (!client.ReadExact(&clientInit, sz_rfbClientInitMsg)) return false;
    if (!WriteServerInit(client)) return false;
    rfbSetEncodingsMsg encodings;
    if (!client.ReadExact(&encodings, sz_rfbSetEncodingsMsg)) return false;
    const unsigned int count = Swap16IfLE(encodings.nEncodings);
    std::vector<CARD8> payload(count * sizeof(CARD32));
    return payload.empty() || client.ReadExact(payload.data(), payload.size());
}

bool WriteUpdateHeader(TcpSocket& client, CARD32 encoding)
{
    rfbFramebufferUpdateRequestMsg request;
    if (!client.ReadExact(&request, sz_rfbFramebufferUpdateRequestMsg)) return false;
    rfbFramebufferUpdateMsg update;
    update.type = rfbFramebufferUpdate;
    update.pad = 0;
    update.nRects = Swap16IfLE(static_cast<CARD16>(1));
    rfbFramebufferUpdateRectHeader rect;
    rect.r.x = Swap16IfLE(static_cast<CARD16>(0));
    rect.r.y = Swap16IfLE(static_cast<CARD16>(0));
    rect.r.w = Swap16IfLE(static_cast<CARD16>(4));
    rect.r.h = Swap16IfLE(static_cast<CARD16>(4));
    rect.encoding = Swap32IfLE(static_cast<CARD32>(encoding));
    return client.WriteAll(&update, sz_rfbFramebufferUpdateMsg) &&
           client.WriteAll(&rect, sz_rfbFramebufferUpdateRectHeader);
}

std::vector<CARD8> Compress(const std::vector<CARD8>& input)
{
    uLongf maxSize = compressBound(static_cast<uLong>(input.size()));
    std::vector<CARD8> compressed(maxSize);
    assert(compress2(compressed.data(), &maxSize, input.data(), static_cast<uLong>(input.size()), Z_BEST_SPEED) == Z_OK);
    compressed.resize(maxSize);
    return compressed;
}

template <typename PayloadWriter>
bool RunOneServer(CARD32 encoding, PayloadWriter writer, unsigned short& port)
{
    TcpListener listener;
    if (!listener.Listen("127.0.0.1", 0)) return false;
    port = listener.Port();
    bool serverOk = false;
    std::thread server([&]() {
        TcpSocket client;
        serverOk = listener.Accept(client) && RunHandshake(client) && WriteUpdateHeader(client, encoding) && writer(client);
    });

    ViewerConfig config;
    config.SetHost("127.0.0.1");
    config.SetPort(port);
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    const bool clientOk = session.Connect(config, result, &error) && session.RequestFramebufferUpdate(false, result, &error);
    session.Disconnect();
    server.join();
    assert(clientOk);
    assert(serverOk);
    assert(result.rectangles.size() == 1);
    assert(result.rectangles[0].encoding == encoding);
    assert(result.update.received);
    assert(result.update.width == 8);
    assert(result.update.height == 8);
    assert(result.update.pixels.size() == 8 * 8 * 4);
    return true;
}

} // namespace

int main()
{
    unsigned short port = 0;
    assert(RunOneServer(rfbEncodingZlib, [](TcpSocket& client) {
        const std::vector<CARD8> raw = Solid(0x10, 4, 4);
        const std::vector<CARD8> compressed = Compress(raw);
        rfbZlibHeader header;
        header.nBytes = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZlibHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingRRE, [](TcpSocket& client) {
        rfbRREHeader header;
        header.nSubrects = Swap32IfLE(static_cast<CARD32>(1));
        const std::vector<CARD8> background = Pixel(0x20);
        const std::vector<CARD8> foreground = Pixel(0x30);
        rfbRectangle rect;
        rect.x = Swap16IfLE(static_cast<CARD16>(1));
        rect.y = Swap16IfLE(static_cast<CARD16>(1));
        rect.w = Swap16IfLE(static_cast<CARD16>(2));
        rect.h = Swap16IfLE(static_cast<CARD16>(2));
        return client.WriteAll(&header, sz_rfbRREHeader) &&
               client.WriteAll(background.data(), background.size()) &&
               client.WriteAll(foreground.data(), foreground.size()) &&
               client.WriteAll(&rect, sz_rfbRectangle);
    }, port));

    assert(RunOneServer(rfbEncodingCoRRE, [](TcpSocket& client) {
        rfbRREHeader header;
        header.nSubrects = Swap32IfLE(static_cast<CARD32>(1));
        const std::vector<CARD8> background = Pixel(0x40);
        const std::vector<CARD8> foreground = Pixel(0x50);
        rfbCoRRERectangle rect{1, 1, 2, 2};
        return client.WriteAll(&header, sz_rfbRREHeader) &&
               client.WriteAll(background.data(), background.size()) &&
               client.WriteAll(foreground.data(), foreground.size()) &&
               client.WriteAll(&rect, sz_rfbCoRRERectangle);
    }, port));


    assert(RunOneServer(rfbEncodingZRLE, [](TcpSocket& client) {
        std::vector<CARD8> zrle;
        zrle.push_back(1); // one solid tile
        const std::vector<CARD8> color = CompactPixel(0x80);
        zrle.insert(zrle.end(), color.begin(), color.end());
        const std::vector<CARD8> compressed = Compress(zrle);
        rfbZRLEHeader header;
        header.length = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZRLEHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingZRLE, [](TcpSocket& client) {
        std::vector<CARD8> zrle;
        zrle.push_back(0); // raw compact pixels
        const std::vector<CARD8> color = CompactPixel(0x84);
        for (unsigned int i = 0; i < 16; ++i) {
            zrle.insert(zrle.end(), color.begin(), color.end());
        }
        const std::vector<CARD8> compressed = Compress(zrle);
        rfbZRLEHeader header;
        header.length = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZRLEHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingZRLE, [](TcpSocket& client) {
        std::vector<CARD8> zrle;
        zrle.push_back(2); // two-colour packed palette
        const std::vector<CARD8> first = CompactPixel(0x88);
        const std::vector<CARD8> second = CompactPixel(0x8c);
        zrle.insert(zrle.end(), first.begin(), first.end());
        zrle.insert(zrle.end(), second.begin(), second.end());
        for (unsigned int row = 0; row < 4; ++row) {
            zrle.push_back(0x50); // 0,1,0,1 and row padding
        }
        const std::vector<CARD8> compressed = Compress(zrle);
        rfbZRLEHeader header;
        header.length = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZRLEHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingZRLE, [](TcpSocket& client) {
        std::vector<CARD8> zrle;
        zrle.push_back(128); // plain RLE
        const std::vector<CARD8> color = CompactPixel(0x90);
        zrle.insert(zrle.end(), color.begin(), color.end());
        zrle.push_back(15); // 16 pixels total, encoded as run length - 1
        const std::vector<CARD8> compressed = Compress(zrle);
        rfbZRLEHeader header;
        header.length = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZRLEHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingZRLE, [](TcpSocket& client) {
        std::vector<CARD8> zrle;
        zrle.push_back(130); // palette RLE, two colours
        const std::vector<CARD8> first = CompactPixel(0x94);
        const std::vector<CARD8> second = CompactPixel(0x98);
        zrle.insert(zrle.end(), first.begin(), first.end());
        zrle.insert(zrle.end(), second.begin(), second.end());
        zrle.push_back(0x80);
        zrle.push_back(7); // eight pixels of palette index 0
        zrle.push_back(0x81);
        zrle.push_back(7); // eight pixels of palette index 1
        const std::vector<CARD8> compressed = Compress(zrle);
        rfbZRLEHeader header;
        header.length = Swap32IfLE(static_cast<CARD32>(compressed.size()));
        return client.WriteAll(&header, sz_rfbZRLEHeader) && client.WriteAll(compressed.data(), compressed.size());
    }, port));

    assert(RunOneServer(rfbEncodingHextile, [](TcpSocket& client) {
        CARD8 subencoding = rfbHextileBackgroundSpecified | rfbHextileAnySubrects | rfbHextileSubrectsColoured;
        const std::vector<CARD8> background = Pixel(0x60);
        const std::vector<CARD8> foreground = Pixel(0x70);
        CARD8 subrectCount = 1;
        CARD8 xy = rfbHextilePackXY(1, 1);
        CARD8 wh = rfbHextilePackWH(2, 2);
        return client.WriteAll(&subencoding, sizeof(subencoding)) &&
               client.WriteAll(background.data(), background.size()) &&
               client.WriteAll(&subrectCount, sizeof(subrectCount)) &&
               client.WriteAll(foreground.data(), foreground.size()) &&
               client.WriteAll(&xy, sizeof(xy)) &&
               client.WriteAll(&wh, sizeof(wh));
    }, port));
    return 0;
}
