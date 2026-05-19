// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_TRANSPORT_H
#define UVNC_WINVNC_PORTABLE_RFB_TRANSPORT_H

#include "vncPortableTcp.h"

#include <cstddef>
#include <memory>
#include <string>

namespace uvnc {
namespace winvnc {
namespace portable {

class RfbTransport {
public:
    virtual ~RfbTransport() {}
    virtual bool ReadExact(void *buffer, std::size_t length) = 0;
    virtual bool WriteAll(const void *buffer, std::size_t length) = 0;
};

class TcpRfbTransport : public RfbTransport {
public:
    explicit TcpRfbTransport(TcpSocket& socket);

    bool ReadExact(void *buffer, std::size_t length) override;
    bool WriteAll(const void *buffer, std::size_t length) override;
    TcpSocket& Socket() { return socket_; }

private:
    TcpSocket& socket_;
};

bool CreateOpenSslServerTransport(TcpSocket& socket,
                                  const std::string& certificateFile,
                                  const std::string& privateKeyFile,
                                  std::unique_ptr<RfbTransport>& transport,
                                  std::string *error = nullptr);
bool CreateOpenSslClientTransport(TcpSocket& socket,
                                  const std::string& caFile,
                                  const std::string& serverName,
                                  bool verifyPeer,
                                  std::unique_ptr<RfbTransport>& transport,
                                  std::string *error = nullptr);

bool OpenSslTransportAvailable();

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_TRANSPORT_H
