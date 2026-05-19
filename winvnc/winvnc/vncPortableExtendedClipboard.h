// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_EXTENDED_CLIPBOARD_H
#define UVNC_WINVNC_PORTABLE_EXTENDED_CLIPBOARD_H

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

const CARD32 kExtendedClipboardServerCaps = clipCaps | clipRequest | clipProvide | clipNotify | clipPeek | clipText;
const CARD32 kExtendedClipboardDefaultTextLimit = 10U * 1024U * 1024U;

struct ExtendedClipboardPayload {
    CARD32 flags;
    std::string text;
    bool textPresent;
    bool malformed;

    ExtendedClipboardPayload();
};

bool IsExtendedClipboardWireLength(CARD32 wireLength);
unsigned int ExtendedClipboardPayloadLength(CARD32 wireLength);
std::vector<CARD8> EncodeExtendedClipboardCaps(CARD32 caps = kExtendedClipboardServerCaps,
                                                CARD32 textLimit = kExtendedClipboardDefaultTextLimit);
std::vector<CARD8> EncodeExtendedClipboardNotify(bool textAvailable);
std::vector<CARD8> EncodeExtendedClipboardRequest(CARD32 formats = clipText);
std::vector<CARD8> EncodeExtendedClipboardPeek(CARD32 formats = clipText);
std::vector<CARD8> EncodeExtendedClipboardProvideText(const std::string& text);
bool DecodeExtendedClipboardPayload(const std::vector<CARD8>& payload, ExtendedClipboardPayload& out);
bool DecodeExtendedClipboardProvidedText(const std::vector<CARD8>& payload, std::string& text);
std::vector<CARD8> EncodeExtendedServerCutText(const std::vector<CARD8>& extendedPayload);
std::vector<CARD8> EncodeExtendedClientCutText(const std::vector<CARD8>& extendedPayload);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_EXTENDED_CLIPBOARD_H
