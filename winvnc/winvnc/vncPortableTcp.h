// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_TCP_H
#define UVNC_WINVNC_PORTABLE_TCP_H

#include <cstddef>
#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(int fd);
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;
    ~TcpSocket();

    bool Valid() const;
    int NativeHandle() const { return fd_; }
    void Close();

    bool ReadExact(void *buffer, std::size_t length);
    bool WriteAll(const void *buffer, std::size_t length);

    static bool Connect(const std::string& address, unsigned short port, TcpSocket& socket);

private:
    int fd_;
};

class TcpListener {
public:
    TcpListener();
    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    ~TcpListener();

    bool Listen(const std::string& address, unsigned short port, int backlog = 8);
    bool Valid() const;
    unsigned short Port() const { return port_; }
    int NativeHandle() const { return fd_; }
    void Close();
    bool Accept(TcpSocket& socket);

private:
    int fd_;
    unsigned short port_;
};

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_TCP_H
