// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_CAPTURE_BACKEND_H
#define UVNC_WINVNC_LINUX_CAPTURE_BACKEND_H

#include <string>

namespace uvnc {
namespace winvnc {
namespace linuxfb {

enum class CaptureBackend {
    Auto,
    Memory,
    RawFile,
    X11,
};

bool ParseCaptureBackendName(const std::string& name, CaptureBackend& backend);
const char *CaptureBackendName(CaptureBackend backend);
const char *CaptureBackendDescription(CaptureBackend backend);

bool IsCaptureBackendRuntimeAvailable(CaptureBackend backend, bool hasRawFramebufferFile);
bool ResolveCaptureBackend(CaptureBackend requested,
                           bool hasRawFramebufferFile,
                           CaptureBackend& resolved,
                           std::string *error = nullptr);

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_CAPTURE_BACKEND_H
