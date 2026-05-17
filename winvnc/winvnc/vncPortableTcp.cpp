// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableTcp.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace uvnc {
namespace winvnc {
namespace portable {

namespace {

void CloseFd(int& fd)
{
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        fd = -1;
    }
}

bool FillAddress(const std::string& address, unsigned short port, sockaddr_in& out)
{
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (address.empty() || address == "0.0.0.0") {
        out.sin_addr.s_addr = htonl(INADDR_ANY);
        return true;
    }
    return inet_pton(AF_INET, address.c_str(), &out.sin_addr) == 1;
}

} // namespace

TcpSocket::TcpSocket()
    : fd_(-1)
{
}

TcpSocket::TcpSocket(int fd)
    : fd_(fd)
{
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : fd_(other.fd_)
{
    other.fd_ = -1;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other) {
        Close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

TcpSocket::~TcpSocket()
{
    Close();
}

bool TcpSocket::Valid() const
{
    return fd_ >= 0;
}

void TcpSocket::Close()
{
    CloseFd(fd_);
}

bool TcpSocket::ReadExact(void *buffer, std::size_t length)
{
    char *next = static_cast<char *>(buffer);
    std::size_t remaining = length;
    while (remaining > 0) {
        const ssize_t got = recv(fd_, next, remaining, 0);
        if (got == 0) {
            return false;
        }
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        next += got;
        remaining -= static_cast<std::size_t>(got);
    }
    return true;
}

bool TcpSocket::WriteAll(const void *buffer, std::size_t length)
{
    const char *next = static_cast<const char *>(buffer);
    std::size_t remaining = length;
    while (remaining > 0) {
        const ssize_t sent = send(fd_, next, remaining, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        next += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
    return true;
}

bool TcpSocket::Connect(const std::string& address, unsigned short port, TcpSocket& socket)
{
    socket.Close();
    sockaddr_in target;
    if (!FillAddress(address, port, target)) {
        return false;
    }
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    if (connect(fd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) != 0) {
        close(fd);
        return false;
    }
    socket = TcpSocket(fd);
    return true;
}

TcpListener::TcpListener()
    : fd_(-1),
      port_(0)
{
}

TcpListener::~TcpListener()
{
    Close();
}

bool TcpListener::Listen(const std::string& address, unsigned short port, int backlog)
{
    Close();
    sockaddr_in bindAddress;
    if (!FillAddress(address, port, bindAddress)) {
        return false;
    }
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        return false;
    }
    int reuse = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (bind(fd_, reinterpret_cast<sockaddr *>(&bindAddress), sizeof(bindAddress)) != 0) {
        Close();
        return false;
    }
    if (listen(fd_, backlog) != 0) {
        Close();
        return false;
    }
    sockaddr_in actual;
    socklen_t length = sizeof(actual);
    if (getsockname(fd_, reinterpret_cast<sockaddr *>(&actual), &length) == 0) {
        port_ = ntohs(actual.sin_port);
    } else {
        port_ = port;
    }
    return true;
}

bool TcpListener::Valid() const
{
    return fd_ >= 0;
}

void TcpListener::Close()
{
    CloseFd(fd_);
    port_ = 0;
}

bool TcpListener::Accept(TcpSocket& socket)
{
    socket.Close();
    for (;;) {
        const int accepted = accept(fd_, nullptr, nullptr);
        if (accepted >= 0) {
            socket = TcpSocket(accepted);
            return true;
        }
        if (errno == EINTR) {
            continue;
        }
        return false;
    }
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
