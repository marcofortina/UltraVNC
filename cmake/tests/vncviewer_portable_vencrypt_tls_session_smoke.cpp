// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableMemoryServer.h"
#include "vncPortableViewerConfig.h"
#include "vncPortableViewerSession.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

using namespace uvnc::vncviewer::portable;
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
Y4nEnxD2jr7KO31bSi8cfgVtpGFaZAYj37Yw6WW7AAt48GfyL/PnoPYzJ9ha3G5
xJtmo3cKBnxyGa4/Tkw4qK0dveFag9IKDYtIm+lOuwKBgQCmn46qGDHZ8tiq1t0H
jC+mLaRuzpYuqAUO/y0iBhDkw8z4p90AmvsmHJya5xdWqyXjXjMD0EAB9heTt7nR
+wLmfOEBMM/3y4J2HZEOibxxP8abaIaJVuWSDH91jXgki8E59F7qbTvd4rVxXNWz
iHjgL4rvNX0RzzPTY+1x0BI8HA==
-----END PRIVATE KEY-----
)PEM";

std::string WriteTempFile(const std::string& name, const char *content)
{
    std::string tmpl = "/tmp/uvnc-viewer-tls-XXXXXX";
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

} // namespace

int main()
{
    const std::string certPath = WriteTempFile("server.crt", kCertificate);
    const std::string keyPath = WriteTempFile("server.key", kPrivateKey);

    ServerConfig serverConfig;
    serverConfig.SetBindAddress("127.0.0.1");
    serverConfig.SetPort(0);
    serverConfig.SetSize(64, 32);
    serverConfig.SetDesktopName("viewer-tls-vencrypt-smoke");
    serverConfig.SetAuthMode(ServerAuthMode::VncPassword);
    serverConfig.SetVncPassword("secret");
    serverConfig.SetTransportSecurity(TransportSecurityMode::VeNCryptX509Vnc);
    serverConfig.SetTlsCertificateFile(certPath);
    serverConfig.SetTlsPrivateKeyFile(keyPath);

    MemoryServer server;
    assert(server.Start(serverConfig));
    std::thread worker([&]() { assert(server.ServeOne()); });

    ViewerConfig viewerConfig;
    viewerConfig.SetHost("localhost");
    viewerConfig.SetPort(server.Port());
    viewerConfig.SetPassword("secret");
    viewerConfig.SetTransportSecurity(ViewerTransportSecurityMode::VeNCryptX509Vnc);
    viewerConfig.SetTlsCaFile(certPath);

    ViewerSessionResult result;
    std::string error;
    assert(ViewerSession().RunHandshake(viewerConfig, result, &error));
    assert(error.empty());
    assert(result.width == 64);
    assert(result.height == 32);
    assert(result.desktopName == "viewer-tls-vencrypt-smoke");

    worker.join();
    server.Stop();
    return 0;
}
