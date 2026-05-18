// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableTcp.h"

#include <cassert>

using uvnc::winvnc::portable::TcpListener;
using uvnc::winvnc::portable::TcpSocket;

int main()
{
    TcpListener listener;
    assert(listener.Listen("127.0.0.1", 0));

    TcpSocket socket;
    assert(!listener.AcceptWithTimeoutMs(socket, 10));
    assert(!socket.Valid());

    listener.Close();
    return 0;
}
