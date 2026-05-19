// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_CLIPBOARD_BACKEND_H
#define UVNC_WINVNC_LINUX_CLIPBOARD_BACKEND_H

#include "vncPortableRfbSession.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxclipboard {

class X11ClipboardBackend : public portable::RfbClipboardSink {
public:
    bool SetText(const std::string& text, std::string *error = nullptr) override;
    bool GetText(std::string& text, std::string *error = nullptr) const;

    static bool RuntimeAvailable(std::string *reason = nullptr);
};

} // namespace linuxclipboard
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_CLIPBOARD_BACKEND_H
