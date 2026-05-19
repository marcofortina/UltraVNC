// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableRfbTransport.h"

#ifdef UVNC_HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include <cstring>
#include <sstream>

namespace uvnc {
namespace winvnc {
namespace portable {

TcpRfbTransport::TcpRfbTransport(TcpSocket& socket)
    : socket_(socket)
{
}

bool TcpRfbTransport::ReadExact(void *buffer, std::size_t length)
{
    return socket_.ReadExact(buffer, length);
}

bool TcpRfbTransport::WriteAll(const void *buffer, std::size_t length)
{
    return socket_.WriteAll(buffer, length);
}

#ifdef UVNC_HAVE_OPENSSL
namespace {

std::string LastOpenSslError(const char *context)
{
    const unsigned long code = ERR_get_error();
    std::ostringstream out;
    out << context;
    if (code != 0) {
        char buffer[256] = {};
        ERR_error_string_n(code, buffer, sizeof(buffer));
        out << ": " << buffer;
    }
    return out.str();
}

class OpenSslTransport : public RfbTransport {
public:
    OpenSslTransport(SSL_CTX *context, SSL *ssl)
        : context_(context), ssl_(ssl)
    {
    }

    ~OpenSslTransport() override
    {
        if (ssl_) {
            SSL_shutdown(ssl_);
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (context_) {
            SSL_CTX_free(context_);
            context_ = nullptr;
        }
    }

    bool ReadExact(void *buffer, std::size_t length) override
    {
        unsigned char *next = static_cast<unsigned char *>(buffer);
        std::size_t remaining = length;
        while (remaining > 0) {
            const int chunk = remaining > static_cast<std::size_t>(0x7fffffff) ? 0x7fffffff : static_cast<int>(remaining);
            const int got = SSL_read(ssl_, next, chunk);
            if (got <= 0) {
                const int error = SSL_get_error(ssl_, got);
                if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                return false;
            }
            next += got;
            remaining -= static_cast<std::size_t>(got);
        }
        return true;
    }

    bool WriteAll(const void *buffer, std::size_t length) override
    {
        const unsigned char *next = static_cast<const unsigned char *>(buffer);
        std::size_t remaining = length;
        while (remaining > 0) {
            const int chunk = remaining > static_cast<std::size_t>(0x7fffffff) ? 0x7fffffff : static_cast<int>(remaining);
            const int sent = SSL_write(ssl_, next, chunk);
            if (sent <= 0) {
                const int error = SSL_get_error(ssl_, sent);
                if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                return false;
            }
            next += sent;
            remaining -= static_cast<std::size_t>(sent);
        }
        return true;
    }

private:
    SSL_CTX *context_;
    SSL *ssl_;
};

} // namespace
#endif // UVNC_HAVE_OPENSSL

bool OpenSslTransportAvailable()
{
#ifdef UVNC_HAVE_OPENSSL
    return true;
#else
    return false;
#endif
}

bool CreateOpenSslServerTransport(TcpSocket& socket,
                                  const std::string& certificateFile,
                                  const std::string& privateKeyFile,
                                  std::unique_ptr<RfbTransport>& transport,
                                  std::string *error)
{
    transport.reset();
#ifdef UVNC_HAVE_OPENSSL
    SSL_CTX *context = SSL_CTX_new(TLS_server_method());
    if (!context) {
        if (error) *error = LastOpenSslError("cannot create TLS server context");
        return false;
    }
    SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);
    if (SSL_CTX_use_certificate_file(context, certificateFile.c_str(), SSL_FILETYPE_PEM) != 1) {
        if (error) *error = LastOpenSslError("cannot load TLS certificate");
        SSL_CTX_free(context);
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(context, privateKeyFile.c_str(), SSL_FILETYPE_PEM) != 1) {
        if (error) *error = LastOpenSslError("cannot load TLS private key");
        SSL_CTX_free(context);
        return false;
    }
    if (SSL_CTX_check_private_key(context) != 1) {
        if (error) *error = LastOpenSslError("TLS private key does not match certificate");
        SSL_CTX_free(context);
        return false;
    }
    SSL *ssl = SSL_new(context);
    if (!ssl) {
        if (error) *error = LastOpenSslError("cannot create TLS session");
        SSL_CTX_free(context);
        return false;
    }
    SSL_set_fd(ssl, socket.NativeHandle());
    if (SSL_accept(ssl) != 1) {
        if (error) *error = LastOpenSslError("TLS server handshake failed");
        SSL_free(ssl);
        SSL_CTX_free(context);
        return false;
    }
    transport.reset(new OpenSslTransport(context, ssl));
    return true;
#else
    (void)socket;
    (void)certificateFile;
    (void)privateKeyFile;
    if (error) *error = "OpenSSL support is not compiled in";
    return false;
#endif
}

bool CreateOpenSslClientTransport(TcpSocket& socket,
                                  const std::string& caFile,
                                  const std::string& serverName,
                                  bool verifyPeer,
                                  std::unique_ptr<RfbTransport>& transport,
                                  std::string *error)
{
    transport.reset();
#ifdef UVNC_HAVE_OPENSSL
    SSL_CTX *context = SSL_CTX_new(TLS_client_method());
    if (!context) {
        if (error) *error = LastOpenSslError("cannot create TLS client context");
        return false;
    }
    SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);
    if (verifyPeer) {
        if (caFile.empty()) {
            if (error) *error = "TLS peer verification requires a CA file";
            SSL_CTX_free(context);
            return false;
        }
        if (SSL_CTX_load_verify_locations(context, caFile.c_str(), nullptr) != 1) {
            if (error) *error = LastOpenSslError("cannot load TLS CA file");
            SSL_CTX_free(context);
            return false;
        }
        SSL_CTX_set_verify(context, SSL_VERIFY_PEER, nullptr);
    } else {
        SSL_CTX_set_verify(context, SSL_VERIFY_NONE, nullptr);
    }

    SSL *ssl = SSL_new(context);
    if (!ssl) {
        if (error) *error = LastOpenSslError("cannot create TLS client session");
        SSL_CTX_free(context);
        return false;
    }
    if (!serverName.empty()) {
        SSL_set_tlsext_host_name(ssl, serverName.c_str());
        if (verifyPeer) {
            X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
            X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            if (X509_VERIFY_PARAM_set1_host(param, serverName.c_str(), 0) != 1) {
                if (error) *error = LastOpenSslError("cannot configure TLS hostname verification");
                SSL_free(ssl);
                SSL_CTX_free(context);
                return false;
            }
        }
    }
    SSL_set_fd(ssl, socket.NativeHandle());
    if (SSL_connect(ssl) != 1) {
        if (error) *error = LastOpenSslError("TLS client handshake failed");
        SSL_free(ssl);
        SSL_CTX_free(context);
        return false;
    }
    if (verifyPeer && SSL_get_verify_result(ssl) != X509_V_OK) {
        if (error) *error = "TLS peer certificate verification failed";
        SSL_free(ssl);
        SSL_CTX_free(context);
        return false;
    }
    transport.reset(new OpenSslTransport(context, ssl));
    return true;
#else
    (void)socket;
    (void)caFile;
    (void)serverName;
    (void)verifyPeer;
    if (error) *error = "OpenSSL support is not compiled in";
    return false;
#endif
}


} // namespace portable
} // namespace winvnc
} // namespace uvnc
