// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableCursor.h"

#include <cstring>

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

void AppendBytes(std::vector<CARD8>& out, const void *data, std::size_t size)
{
    const std::size_t oldSize = out.size();
    out.resize(oldSize + size);
    std::memcpy(out.data() + oldSize, data, size);
}

void AppendBit(std::vector<CARD8>& bits, unsigned int index, bool set)
{
    if (!set) {
        return;
    }
    bits[index / 8] |= static_cast<CARD8>(0x80u >> (index % 8));
}

std::vector<CARD8> CursorMaskBytes(const CursorShape& shape)
{
    const unsigned int stride = (shape.width + 7) / 8;
    std::vector<CARD8> mask(stride * shape.height, 0);
    for (unsigned int y = 0; y < shape.height; ++y) {
        for (unsigned int x = 0; x < shape.width; ++x) {
            const std::size_t pixel = (static_cast<std::size_t>(y) * shape.width + x) * 4;
            AppendBit(mask, y * stride * 8 + x, shape.bgra[pixel + 3] != 0);
        }
    }
    return mask;
}

} // namespace

CursorShape::CursorShape()
    : width(0),
      height(0),
      hotspotX(0),
      hotspotY(0),
      bgra()
{
}

bool CursorShape::Valid() const
{
    if (width == 0 || height == 0) {
        return bgra.empty();
    }
    return bgra.size() == static_cast<std::size_t>(width) * height * 4 &&
           hotspotX < width && hotspotY < height;
}

CursorShape EmptyCursorShape()
{
    return CursorShape();
}

CursorShape DefaultArrowCursorShape()
{
    CursorShape shape;
    shape.width = 16;
    shape.height = 16;
    shape.hotspotX = 0;
    shape.hotspotY = 0;
    shape.bgra.assign(static_cast<std::size_t>(shape.width) * shape.height * 4, 0);

    static const char *rows[] = {
        "X...............",
        "XX..............",
        "XOX.............",
        "XOOX............",
        "XOOOX...........",
        "XOOOOX..........",
        "XOOOOOX.........",
        "XOOOOOOX........",
        "XOOOOOOOX.......",
        "XOOOOX..........",
        "XOXXOOX.........",
        "XX..XOOX........",
        "X....XOOX.......",
        ".....XOOX.......",
        "......XX........",
        "................",
    };

    for (unsigned int y = 0; y < shape.height; ++y) {
        for (unsigned int x = 0; x < shape.width; ++x) {
            const char p = rows[y][x];
            if (p == '.') {
                continue;
            }
            const std::size_t offset = (static_cast<std::size_t>(y) * shape.width + x) * 4;
            const CARD8 value = p == 'X' ? 0 : 255;
            shape.bgra[offset + 0] = value;
            shape.bgra[offset + 1] = value;
            shape.bgra[offset + 2] = value;
            shape.bgra[offset + 3] = 255;
        }
    }
    return shape;
}

std::vector<CARD8> EncodeRichCursorShapeUpdate(const CursorShape& shape)
{
    if (!shape.Valid() || shape.width > 65535 || shape.height > 65535 || shape.hotspotX > 65535 || shape.hotspotY > 65535) {
        return std::vector<CARD8>();
    }
    if (shape.width == 0 || shape.height == 0) {
        return EncodeEmptyCursorShapeUpdate(rfbEncodingRichCursor);
    }

    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.r.x = Swap16IfLE(static_cast<CARD16>(shape.hotspotX));
    header.r.y = Swap16IfLE(static_cast<CARD16>(shape.hotspotY));
    header.r.w = Swap16IfLE(static_cast<CARD16>(shape.width));
    header.r.h = Swap16IfLE(static_cast<CARD16>(shape.height));
    header.encoding = Swap32IfLE(rfbEncodingRichCursor);

    std::vector<CARD8> bytes;
    bytes.reserve(sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader + shape.bgra.size());
    AppendBytes(bytes, &update, sz_rfbFramebufferUpdateMsg);
    AppendBytes(bytes, &header, sz_rfbFramebufferUpdateRectHeader);
    AppendBytes(bytes, shape.bgra.data(), shape.bgra.size());
    const std::vector<CARD8> mask = CursorMaskBytes(shape);
    AppendBytes(bytes, mask.data(), mask.size());
    return bytes;
}

std::vector<CARD8> EncodeXCursorShapeUpdate(const CursorShape& shape)
{
    if (!shape.Valid() || shape.width > 65535 || shape.height > 65535 || shape.hotspotX > 65535 || shape.hotspotY > 65535) {
        return std::vector<CARD8>();
    }
    if (shape.width == 0 || shape.height == 0) {
        return EncodeEmptyCursorShapeUpdate(rfbEncodingXCursor);
    }

    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.r.x = Swap16IfLE(static_cast<CARD16>(shape.hotspotX));
    header.r.y = Swap16IfLE(static_cast<CARD16>(shape.hotspotY));
    header.r.w = Swap16IfLE(static_cast<CARD16>(shape.width));
    header.r.h = Swap16IfLE(static_cast<CARD16>(shape.height));
    header.encoding = Swap32IfLE(rfbEncodingXCursor);

    rfbXCursorColors colors;
    colors.foreRed = 0;
    colors.foreGreen = 0;
    colors.foreBlue = 0;
    colors.backRed = 255;
    colors.backGreen = 255;
    colors.backBlue = 255;

    const unsigned int stride = (shape.width + 7) / 8;
    std::vector<CARD8> data(stride * shape.height, 0);
    std::vector<CARD8> mask(stride * shape.height, 0);
    for (unsigned int y = 0; y < shape.height; ++y) {
        for (unsigned int x = 0; x < shape.width; ++x) {
            const std::size_t pixel = (static_cast<std::size_t>(y) * shape.width + x) * 4;
            const bool visible = shape.bgra[pixel + 3] != 0;
            const unsigned int bit = y * stride * 8 + x;
            AppendBit(mask, bit, visible);
            const unsigned int luma = shape.bgra[pixel] + shape.bgra[pixel + 1] + shape.bgra[pixel + 2];
            AppendBit(data, bit, visible && luma < 384);
        }
    }

    std::vector<CARD8> bytes;
    AppendBytes(bytes, &update, sz_rfbFramebufferUpdateMsg);
    AppendBytes(bytes, &header, sz_rfbFramebufferUpdateRectHeader);
    AppendBytes(bytes, &colors, sz_rfbXCursorColors);
    AppendBytes(bytes, data.data(), data.size());
    AppendBytes(bytes, mask.data(), mask.size());
    return bytes;
}

std::vector<CARD8> EncodeEmptyCursorShapeUpdate(CARD32 encoding)
{
    if (encoding != rfbEncodingXCursor && encoding != rfbEncodingRichCursor) {
        return std::vector<CARD8>();
    }
    rfbFramebufferUpdateMsg update;
    std::memset(&update, 0, sizeof(update));
    update.type = rfbFramebufferUpdate;
    update.nRects = Swap16IfLE(1);

    rfbFramebufferUpdateRectHeader header;
    std::memset(&header, 0, sizeof(header));
    header.encoding = Swap32IfLE(encoding);

    std::vector<CARD8> bytes(sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader);
    std::memcpy(bytes.data(), &update, sz_rfbFramebufferUpdateMsg);
    std::memcpy(bytes.data() + sz_rfbFramebufferUpdateMsg, &header, sz_rfbFramebufferUpdateRectHeader);
    return bytes;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc