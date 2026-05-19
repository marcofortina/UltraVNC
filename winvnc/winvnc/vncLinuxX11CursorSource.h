// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_X11_CURSOR_SOURCE_H
#define UVNC_WINVNC_LINUX_X11_CURSOR_SOURCE_H

#include "vncPortableRfbSession.h"

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

class X11CursorSource : public portable::RfbCursorSource {
public:
    explicit X11CursorSource(const std::string& displayName = std::string());
    ~X11CursorSource() override;

    bool GetCursorShape(portable::CursorShape& shape, std::string *error = nullptr) const override;

    static bool IsBuildAvailable();
    static bool IsRuntimeAvailable(const std::string& displayName = std::string(), std::string *reason = nullptr);
    static const char *UnavailableReason();

private:
    bool EnsureDisplay(std::string *error) const;

    std::string displayName_;
    mutable void *display_;
};

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_X11_CURSOR_SOURCE_H
