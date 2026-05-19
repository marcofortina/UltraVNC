// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableExtendedClipboard.h"

#include <cassert>
#include <cstring>

using namespace uvnc::winvnc::portable;

namespace {

rfbServerCutTextMsg ReadHeader(const std::vector<CARD8>& bytes)
{
    rfbServerCutTextMsg header;
    std::memset(&header, 0, sizeof(header));
    assert(bytes.size() >= sz_rfbServerCutTextMsg);
    std::memcpy(&header, bytes.data(), sz_rfbServerCutTextMsg);
    return header;
}

} // namespace

int main()
{
    const std::vector<CARD8> caps = EncodeExtendedClipboardCaps();
    ExtendedClipboardPayload parsed;
    assert(DecodeExtendedClipboardPayload(caps, parsed));
    assert((parsed.flags & clipCaps) != 0);
    assert((parsed.flags & clipText) != 0);

    const std::vector<CARD8> notify = EncodeExtendedClipboardNotify(true);
    assert(DecodeExtendedClipboardPayload(notify, parsed));
    assert((parsed.flags & clipNotify) != 0);
    assert((parsed.flags & clipText) != 0);

    const std::vector<CARD8> provide = EncodeExtendedClipboardProvideText("extended text");
    assert(DecodeExtendedClipboardPayload(provide, parsed));
    assert((parsed.flags & clipProvide) != 0);
    assert(parsed.textPresent);
    assert(parsed.text == "extended text");

    const std::vector<CARD8> cut = EncodeExtendedServerCutText(provide);
    const rfbServerCutTextMsg serverHeader = ReadHeader(cut);
    assert(serverHeader.type == rfbServerCutText);
    assert(IsExtendedClipboardWireLength(serverHeader.length));
    assert(ExtendedClipboardPayloadLength(serverHeader.length) == provide.size());

    const std::vector<CARD8> clientCut = EncodeExtendedClientCutText(EncodeExtendedClipboardRequest());
    const rfbServerCutTextMsg clientHeader = ReadHeader(clientCut);
    assert(clientHeader.type == rfbClientCutText);
    assert(IsExtendedClipboardWireLength(clientHeader.length));
    return 0;
}
