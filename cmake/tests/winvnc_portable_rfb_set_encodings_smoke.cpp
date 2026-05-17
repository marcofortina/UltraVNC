// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbMessages.h"

#include <cassert>
#include <cstring>
#include <vector>

using namespace uvnc::winvnc::portable;

int main()
{
    std::vector<CARD32> requested;
    requested.push_back(rfbEncodingRaw);
    requested.push_back(rfbEncodingHextile);
    requested.push_back(rfbEncodingZRLE);

    const std::vector<CARD8> wire = EncodeSetEncodings(requested);
    assert(wire.size() == sz_rfbSetEncodingsMsg + requested.size() * sizeof(CARD32));

    rfbSetEncodingsMsg header;
    std::memcpy(&header, wire.data(), sizeof(header));
    unsigned int count = 0;
    assert(DecodeSetEncodingsHeader(header, count));
    assert(count == requested.size());

    const std::vector<CARD8> payload(wire.begin() + sz_rfbSetEncodingsMsg, wire.end());
    const std::vector<CARD32> decoded = DecodeSetEncodingsPayload(payload);
    assert(decoded == requested);

    header.type = rfbKeyEvent;
    assert(!DecodeSetEncodingsHeader(header, count));

    std::vector<CARD8> invalid(3, 0);
    assert(DecodeSetEncodingsPayload(invalid).empty());
    return 0;
}
