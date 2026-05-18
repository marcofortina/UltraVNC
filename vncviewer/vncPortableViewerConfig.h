// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_PORTABLE_VIEWER_CONFIG_H
#define UVNC_VNCVIEWER_PORTABLE_VIEWER_CONFIG_H

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace portable {

class ViewerConfig {
public:
    ViewerConfig();

    const std::string& Host() const { return host_; }
    unsigned short Port() const { return port_; }
    bool Shared() const { return shared_; }
    bool RequestUpdate() const { return requestUpdate_; }
    bool ViewOnly() const { return viewOnly_; }
    const std::string& Password() const { return password_; }
    bool ContinuousUpdates() const { return continuousUpdates_; }
    unsigned int UpdateIntervalMs() const { return updateIntervalMs_; }
    const std::vector<unsigned int>& Encodings() const { return encodings_; }

    void SetHost(const std::string& host) { host_ = host; }
    void SetPort(unsigned short port) { port_ = port; }
    void SetShared(bool shared) { shared_ = shared; }
    void SetRequestUpdate(bool requestUpdate) { requestUpdate_ = requestUpdate; }
    void SetViewOnly(bool viewOnly) { viewOnly_ = viewOnly; }
    void SetPassword(const std::string& password) { password_ = password; }
    void SetContinuousUpdates(bool continuousUpdates) { continuousUpdates_ = continuousUpdates; }
    void SetUpdateIntervalMs(unsigned int updateIntervalMs) { updateIntervalMs_ = updateIntervalMs; }
    void SetEncodings(const std::vector<unsigned int>& encodings) { encodings_ = encodings; }

    bool Validate(std::string *error = nullptr) const;

private:
    std::string host_;
    unsigned short port_;
    bool shared_;
    bool requestUpdate_;
    bool viewOnly_;
    std::string password_;
    bool continuousUpdates_;
    unsigned int updateIntervalMs_;
    std::vector<unsigned int> encodings_;
};

} // namespace portable
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_PORTABLE_VIEWER_CONFIG_H
