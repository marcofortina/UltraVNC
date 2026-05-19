// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMsLogon.h"

#include <cassert>
#include <string>
#include <vector>

using namespace uvnc::vncviewer::portable;

namespace {

void WriteU64BE(unsigned long long value, std::vector<CARD8>& out)
{
    for (int i = 7; i >= 0; --i) {
        out.push_back(static_cast<CARD8>((value >> (i * 8)) & 0xffULL));
    }
}

} // namespace

int main()
{
    std::vector<CARD8> exchangeBytes;
    WriteU64BE(5, exchangeBytes);
    WriteU64BE(23, exchangeBytes);
    WriteU64BE(8, exchangeBytes);

    MsLogonIIExchange exchange;
    std::string error;
    assert(DecodeMsLogonIIExchange(exchangeBytes, exchange, &error));
    assert(exchange.generator == 5);
    assert(exchange.modulus == 23);
    assert(exchange.serverPublic == 8);

    std::vector<CARD8> response;
    assert(EncodeMsLogonIIResponse(exchange, "LAB\\alice", "secret", response, &error));
    assert(response.size() == kMsLogonIIResponseBytes);
    assert(response[0] != 0 || response[1] != 0 || response[7] != 0);

    assert(!EncodeMsLogonIIResponse(exchange, "", "secret", response, &error));
    assert(error.find("username") != std::string::npos);
    assert(!EncodeMsLogonIIResponse(exchange, "alice", "", response, &error));
    assert(error.find("password") != std::string::npos);
    return 0;
}
