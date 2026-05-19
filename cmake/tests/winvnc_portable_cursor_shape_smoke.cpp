// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableCursor.h"

#include <cassert>
#include <cstring>

using namespace uvnc::winvnc::portable;

int main()
{
    const CursorShape arrow = DefaultArrowCursorShape();
    assert(arrow.Valid());
    assert(arrow.width == 16);
    assert(arrow.height == 16);

    const std::vector<CARD8> rich = EncodeRichCursorShapeUpdate(arrow);
    assert(rich.size() > sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader);
    const auto *richUpdate = reinterpret_cast<const rfbFramebufferUpdateMsg *>(rich.data());
    assert(richUpdate->type == rfbFramebufferUpdate);
    assert(Swap16IfLE(richUpdate->nRects) == 1);
    const auto *richHeader = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(rich.data() + sz_rfbFramebufferUpdateMsg);
    assert(Swap32IfLE(richHeader->encoding) == rfbEncodingRichCursor);
    assert(Swap16IfLE(richHeader->r.w) == 16);
    assert(Swap16IfLE(richHeader->r.h) == 16);

    const std::vector<CARD8> xcursor = EncodeXCursorShapeUpdate(arrow);
    assert(xcursor.size() > sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader + sz_rfbXCursorColors);
    const auto *xHeader = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(xcursor.data() + sz_rfbFramebufferUpdateMsg);
    assert(Swap32IfLE(xHeader->encoding) == rfbEncodingXCursor);
    assert(Swap16IfLE(xHeader->r.w) == 16);
    assert(Swap16IfLE(xHeader->r.h) == 16);

    const std::vector<CARD8> emptyRich = EncodeEmptyCursorShapeUpdate(rfbEncodingRichCursor);
    assert(emptyRich.size() == sz_rfbFramebufferUpdateMsg + sz_rfbFramebufferUpdateRectHeader);
    const auto *emptyHeader = reinterpret_cast<const rfbFramebufferUpdateRectHeader *>(emptyRich.data() + sz_rfbFramebufferUpdateMsg);
    assert(Swap32IfLE(emptyHeader->encoding) == rfbEncodingRichCursor);
    assert(Swap16IfLE(emptyHeader->r.w) == 0);
    assert(Swap16IfLE(emptyHeader->r.h) == 0);

    assert(EncodeEmptyCursorShapeUpdate(rfbEncodingRaw).empty());
    return 0;
}
