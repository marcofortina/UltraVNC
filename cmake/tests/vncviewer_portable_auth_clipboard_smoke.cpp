// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"
#include "vncPortableVncAuth.h"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

using uvnc::vncviewer::portable::EncryptVncAuthChallenge;
using uvnc::winvnc::portable::DecodeClientCutTextHeader;
using uvnc::winvnc::portable::EncodeClientCutText;

int main()
{
    std::vector<unsigned char> challenge(16);
    for (unsigned int i = 0; i < challenge.size(); ++i) {
        challenge[i] = static_cast<unsigned char>(i + 1);
    }
    std::vector<unsigned char> response;
    std::string error;
    assert(EncryptVncAuthChallenge(challenge, "secret", response, &error));
    assert(error.empty());
    assert(response.size() == challenge.size());
    assert(response != challenge);

    std::vector<unsigned char> badChallenge(15);
    assert(!EncryptVncAuthChallenge(badChallenge, "secret", response, &error));
    assert(error == "invalid VNCAuth challenge length");
    assert(!EncryptVncAuthChallenge(challenge, "", response, &error));
    assert(error == "VNCAuth password must not be empty");

    const std::string text = "portable clipboard";
    const std::vector<CARD8> wire = EncodeClientCutText(text);
    assert(wire.size() == sz_rfbClientCutTextMsg + text.size());
    rfbClientCutTextMsg header;
    std::memcpy(&header, wire.data(), sizeof(header));
    unsigned int length = 0;
    assert(DecodeClientCutTextHeader(header, length));
    assert(length == text.size());
    assert(std::string(reinterpret_cast<const char *>(wire.data() + sz_rfbClientCutTextMsg), text.size()) == text);
    header.type = rfbKeyEvent;
    assert(!DecodeClientCutTextHeader(header, length));
    return 0;
}
