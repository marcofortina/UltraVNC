// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxCaptureBackend.h"

#include "vncLinuxX11FramebufferSource.h"

namespace uvnc {
namespace winvnc {
namespace linuxfb {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

} // namespace

bool ParseCaptureBackendName(const std::string& name, CaptureBackend& backend)
{
    if (name == "auto") {
        backend = CaptureBackend::Auto;
        return true;
    }
    if (name == "memory") {
        backend = CaptureBackend::Memory;
        return true;
    }
    if (name == "raw-file") {
        backend = CaptureBackend::RawFile;
        return true;
    }
    if (name == "x11") {
        backend = CaptureBackend::X11;
        return true;
    }
    return false;
}

const char *CaptureBackendName(CaptureBackend backend)
{
    switch (backend) {
    case CaptureBackend::Auto:
        return "auto";
    case CaptureBackend::Memory:
        return "memory";
    case CaptureBackend::RawFile:
        return "raw-file";
    case CaptureBackend::X11:
        return "x11";
    }
    return "unknown";
}

const char *CaptureBackendDescription(CaptureBackend backend)
{
    switch (backend) {
    case CaptureBackend::Auto:
        return "auto-select raw-file when configured, otherwise X11 when available, otherwise memory";
    case CaptureBackend::Memory:
        return "synthetic in-memory framebuffer";
    case CaptureBackend::RawFile:
        return "exact-size raw framebuffer file";
    case CaptureBackend::X11:
        return "X11 root-window capture using XGetImage";
    }
    return "unknown capture backend";
}

bool IsCaptureBackendRuntimeAvailable(CaptureBackend backend, bool hasRawFramebufferFile)
{
    switch (backend) {
    case CaptureBackend::Auto:
    case CaptureBackend::Memory:
        return true;
    case CaptureBackend::RawFile:
        return hasRawFramebufferFile;
    case CaptureBackend::X11:
        return X11DesktopSource::IsAvailable();
    }
    return false;
}

bool ResolveCaptureBackend(CaptureBackend requested,
                           bool hasRawFramebufferFile,
                           CaptureBackend& resolved,
                           std::string *error)
{
    if (requested == CaptureBackend::Auto) {
        if (hasRawFramebufferFile) {
            resolved = CaptureBackend::RawFile;
        } else if (X11DesktopSource::IsAvailable()) {
            resolved = CaptureBackend::X11;
        } else {
            resolved = CaptureBackend::Memory;
        }
        if (error) {
            error->clear();
        }
        return true;
    }

    if (!IsCaptureBackendRuntimeAvailable(requested, hasRawFramebufferFile)) {
        SetError(error, std::string("capture backend is not available: ") + CaptureBackendName(requested));
        return false;
    }

    resolved = requested;
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
