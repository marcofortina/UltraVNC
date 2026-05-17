// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableTcp.h"

#include <cassert>

using namespace uvnc::winvnc::portable;

int main()
{
    TcpSocket socket;
    char byte = 0;
    assert(socket.ReadExact(&byte, 0));
    assert(socket.WriteAll(&byte, 0));
    return 0;
}
