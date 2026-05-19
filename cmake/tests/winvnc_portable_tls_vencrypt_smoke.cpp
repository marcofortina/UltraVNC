// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableTcp.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

extern "C" {
#include "d3des.h"
}

using namespace uvnc::winvnc::portable;

namespace {

const char *kCertificate = R"PEM(-----BEGIN CERTIFICATE-----
MIIDCTCCAfGgAwIBAgIUM0qArGPEcY3fJfdcLf52F70lHjswDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI2MDUxOTA4NDgzNFoXDTI2MDUy
NjA4NDgzNFowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF
AAOCAQ8AMIIBCgKCAQEAsvcklhZNJ3/rfeZZdF6Rkwl0w8mQUFBwYUeBiZzTQxU2
qXrFUvFsnFqc3+gTufSXNxNQlNka+0GeuJTyIrnp3S2JFdSR7ypbgyOuhe7kQdBV
YmogtL3xcFKpkUr+3NT/wySQqjRaKW9QzQRylZv/6899C6pwVmfF/Eb8j+9b6zKV
JQvb+cBpSHBAUkAYN6+moRBmH6mIcC2N0fJis4ZpZfaWAaVvuh5Yy3iXwzzgs0BY
+EF8+bWGed4j3L+eLfxcD0bDR/wSXKdcmJLLDR6Wq5w9PMoQxGz24gaEBpReLSBo
FNsTupTghjnezttmKcT+lOPCsKeb2sv8kvViEArCRwIDAQABo1MwUTAdBgNVHQ4E
FgQUuapFHXmXca6VbQAfRX8XyeHeimgwHwYDVR0jBBgwFoAUuapFHXmXca6VbQAf
RX8XyeHeimgwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAF/Oy
kbQIdDphj8WENjagW/XJ9xq55eja1VshHn04ClMhTraYFbtGbLPSfn3GSYdWUn/F
JVdWp5rgMw8kv0IOZUILnt35ozHttX5NvDSYQkhLizv/eDrHup0WyssVdeYOmKWv
M8+SJHwFeqh0AnIkBauO29f9W1RzOuMzEhLXi1H0Yl80D/5IXQwnanoHxdAzwdKH
mZJqaJcrWXvnmDXSu8AFUDHAJCzmiTuQyZpVARGwH33Vg4WCihnx6faKBpcVVejA
q2WwO4dxSVrj62S1H4IyCvu5G21e4DpJh2s7S6tQ5lts6yUQAuiDYxut02LhA4oJ
4PAOtVW7ZTrbIf8qYw==
-----END CERTIFICATE-----
)PEM";
const char *kPrivateKey = R"PEM(-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQCy9ySWFk0nf+t9
5ll0XpGTCXTDyZBQUHBhR4GJnNNDFTapesVS8WycWpzf6BO59Jc3E1CU2Rr7QZ64
lPIiuendLYkV1JHvKluDI66F7uRB0FViaiC0vfFwUqmRSv7c1P/DJJCqNFopb1DN
BHKVm//rz30LqnBWZ8X8RvyP71vrMpUlC9v5wGlIcEBSQBg3r6ahEGYfqYhwLY3R
8mKzhmll9pYBpW+6HljLeJfDPOCzQFj4QXz5tYZ53iPcv54t/FwPRsNH/BJcp1yY
kssNHparnD08yhDEbPbiBoQGlF4tIGgU2xO6lOCGOd7O22YpxP6U48Kwp5vay/yS
9WIQCsJHAgMBAAECggEATQUAqVkCrl+mjOHQGL1EQDffGQ2Lfo0HuE3qSTuFTgb4
pdqQxl/hQq7aeaAqsSo607iLwutmacB1WvG6/UfuhkH9D7iSb4/Wn2sBRmGnuU08
GeUbmz1thU4F7OIOKiK+yZBYc++g59kguILT/2AKNUVwBs+8lesGMUqpkhZMhDzg
tzLMrtI4Mk/w3rJP0wdPdfBGqJzLLlFG8Oi/x0mFmRPq4uSMJBUXFNOAJ8nQT++r
kVA6fRU7hjXi94RP/LBcEnlGvR7JInxlCLu4XXzV7sdEtfKxPOxdyBKzYfjg/KSI
C19LIYArC3jQMwdM7RLOQ2pvQocX5WdvNzkiDiM8+QKBgQDvyfeYRsQSMfU8qtN/
jFVyrw47VnI+Kxu35f8WWe/hA4rFPxKqojZDnBJNf0yu7zIpcgG/Z7sgalPj1TpF
z0KnyIoAO9A10ukg1l3kvMpHSrTSo35FjKAT03gTTydpUDEO+9fS8WFjMkBgb4ma
pztXIsgndQPqxF0yoCi3zm0ziQKBgQC/EIBc5i7WZmTWrIA6kcqhQI4PDt/gJm6e
VJ/IdmvqLv0YZgimx/CfUCbQNKT/qTKxCb9hqsvFeV1Rywg4XIs7/sQe/nz592ND
bDC0lNZh7OrQ0AYrmwLDebpmBKkHtdeOgU3NrJydKtdDrPXggnsshREmCWSUGy5t
BD76M0NDTwKBgQCKBLwowAK3Xl4Dr2fRMJs2SaBtcxKKyhFIRnAPE3FJGNrVMbqy
0G9fdwPp623d+vvqcx6iZziELe9fYioKaIO7Q2h7PfJYKK/bIMgmkqvzYQK3gzd5
HLo+7ydcJeFPcsLqFvdhCWK+z/vSBiE0DTHQs8p1O5snlCL4ssr9ESo10QKBgQCs
qyIGUrPDaNf6tfxQcg84eVmovB2QucrAZePcy0Cta/epBBUPfKO1pj7dbKYssmEw
Y4nEnxD2jr7KO31bSi8+cfgVtpGFaZAYj37Yw6WW7AAt48GfyL/PnoPYzJ9ha3G5
xJtmo3cKBnxyGa4/Tkw4qK0dveFag9IKDYtIm+lOuwKBgQCmn46qGDHZ8tiq1t0H
jC+mLaRuzpYuqAUO/y0iBhDkw8z4p90AmvsmHJya5xdWqyXjXjMD0EAB9heTt7nR
+wLmfOEBMM/3y4J2HZEOibxxP8abaIaJVuWSDH91jXgki8E59F7qbTvd4rVxXNWz
iHjgL4rvNX0RzzPTY+1x0BI8HA==
-----END PRIVATE KEY-----
)PEM";

std::string WriteTempFile(const std::string& name, const char *content)
{
    std::string tmpl = "/tmp/uvnc-tls-XXXXXX";
    std::vector<char> buffer(tmpl.begin(), tmpl.end());
    buffer.push_back('\0');
    char *dir = mkdtemp(buffer.data());
    assert(dir != nullptr);
    const std::string path = std::string(dir) + "/" + name;
    std::ofstream out(path.c_str(), std::ios::binary);
    out << content;
    out.close();
    chmod(path.c_str(), 0600);
    return path;
}

void EncryptChallenge(std::vector<CARD8>& challenge, const std::string& password)
{
    unsigned char key[8] = {};
    for (std::size_t i = 0; i < sizeof(key) && i < password.size(); ++i) {
        key[i] = static_cast<unsigned char>(password[i]);
    }
    deskey(key, EN0);
    for (std::size_t i = 0; i + 8 <= challenge.size(); i += 8) {
        des(challenge.data() + i, challenge.data() + i);
    }
}

bool ReadExactSsl(SSL *ssl, void *buffer, std::size_t length)
{
    unsigned char *next = static_cast<unsigned char *>(buffer);
    std::size_t remaining = length;
    while (remaining > 0) {
        const int got = SSL_read(ssl, next, static_cast<int>(remaining));
        if (got <= 0) return false;
        next += got;
        remaining -= static_cast<std::size_t>(got);
    }
    return true;
}

bool WriteAllSsl(SSL *ssl, const void *buffer, std::size_t length)
{
    const unsigned char *next = static_cast<const unsigned char *>(buffer);
    std::size_t remaining = length;
    while (remaining > 0) {
        const int sent = SSL_write(ssl, next, static_cast<int>(remaining));
        if (sent <= 0) return false;
        next += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
    return true;
}

} // namespace

int main()
{
    const std::string certPath = WriteTempFile("server.crt", kCertificate);
    const std::string keyPath = WriteTempFile("server.key", kPrivateKey);

    ServerConfig config;
    config.SetBindAddress("127.0.0.1");
    config.SetPort(0);
    config.SetSize(64, 32);
    config.SetDesktopName("tls-vencrypt-smoke");
    config.SetAuthMode(ServerAuthMode::VncPassword);
    config.SetVncPassword("secret");
    config.SetTransportSecurity(TransportSecurityMode::VeNCryptX509Vnc);
    config.SetTlsCertificateFile(certPath);
    config.SetTlsPrivateKeyFile(keyPath);

    MemoryServer server;
    assert(server.Start(config));
    std::thread worker([&]() {
        assert(server.ServeOne());
    });

    TcpSocket client;
    assert(TcpSocket::Connect("127.0.0.1", server.Port(), client));

    char version[sz_rfbProtocolVersionMsg] = {};
    assert(client.ReadExact(version, sizeof(version)));
    assert(std::string(version, sizeof(version)) == ProtocolVersion38());
    assert(client.WriteAll(version, sizeof(version)));

    CARD8 count = 0;
    assert(client.ReadExact(&count, sizeof(count)));
    assert(count == 1);
    CARD8 security = 0;
    assert(client.ReadExact(&security, sizeof(security)));
    assert(security == rfbVeNCypt);
    assert(client.WriteAll(&security, sizeof(security)));

    CARD16 vencryptVersion = 0;
    assert(client.ReadExact(&vencryptVersion, sizeof(vencryptVersion)));
    assert(Swap16IfLE(vencryptVersion) == kVeNCryptVersion);
    assert(client.WriteAll(&vencryptVersion, sizeof(vencryptVersion)));
    CARD8 status = 1;
    assert(client.ReadExact(&status, sizeof(status)));
    assert(status == 0);
    CARD8 subtypeCount = 0;
    assert(client.ReadExact(&subtypeCount, sizeof(subtypeCount)));
    assert(subtypeCount == 1);
    CARD32 subtype = 0;
    assert(client.ReadExact(&subtype, sizeof(subtype)));
    assert(Swap32IfLE(subtype) == kVeNCryptSubTypeX509Vnc);
    assert(client.WriteAll(&subtype, sizeof(subtype)));
    CARD8 accepted = 0;
    assert(client.ReadExact(&accepted, sizeof(accepted)));
    assert(accepted == 1);

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    assert(ctx != nullptr);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL *ssl = SSL_new(ctx);
    assert(ssl != nullptr);
    SSL_set_fd(ssl, client.NativeHandle());
    assert(SSL_connect(ssl) == 1);

    std::vector<CARD8> challenge(16);
    assert(ReadExactSsl(ssl, challenge.data(), challenge.size()));
    EncryptChallenge(challenge, "secret");
    assert(WriteAllSsl(ssl, challenge.data(), challenge.size()));
    CARD32 auth = 1;
    assert(ReadExactSsl(ssl, &auth, sizeof(auth)));
    assert(Swap32IfLE(auth) == rfbVncAuthOK);

    rfbClientInitMsg clientInit;
    clientInit.flags = clientInitShared;
    assert(WriteAllSsl(ssl, &clientInit, sz_rfbClientInitMsg));
    rfbServerInitMsg init;
    assert(ReadExactSsl(ssl, &init, sz_rfbServerInitMsg));
    assert(Swap16IfLE(init.framebufferWidth) == 64);
    assert(Swap16IfLE(init.framebufferHeight) == 32);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    worker.join();
    server.Stop();
    return 0;
}
