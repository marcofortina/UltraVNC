// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxClipboardBackend.h"

#include <cstdlib>

#if defined(UVNC_HAVE_X11)
#include <X11/Xlib.h>
#endif

namespace uvnc {
namespace winvnc {
namespace linuxclipboard {

namespace {

bool HasDisplayEnvironment()
{
    const char *display = std::getenv("DISPLAY");
    return display != nullptr && display[0] != '\0';
}

} // namespace

bool X11ClipboardBackend::RuntimeAvailable(std::string *reason)
{
#if defined(UVNC_HAVE_X11)
    if (!HasDisplayEnvironment()) {
        if (reason) *reason = "DISPLAY is not set";
        return false;
    }
    Display *display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        if (reason) *reason = "cannot open X11 display";
        return false;
    }
    XCloseDisplay(display);
    return true;
#else
    if (reason) *reason = "X11 clipboard backend was not built";
    return false;
#endif
}

bool X11ClipboardBackend::SetText(const std::string& text, std::string *error)
{
#if defined(UVNC_HAVE_X11)
    if (!HasDisplayEnvironment()) {
        if (error) *error = "DISPLAY is not set";
        return false;
    }
    Display *display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        if (error) *error = "cannot open X11 display";
        return false;
    }
    XStoreBuffer(display, text.data(), static_cast<int>(text.size()), 0);
    XFlush(display);
    XCloseDisplay(display);
    return true;
#else
    if (error) *error = "X11 clipboard backend was not built";
    return false;
#endif
}

bool X11ClipboardBackend::GetText(std::string& text, std::string *error) const
{
#if defined(UVNC_HAVE_X11)
    text.clear();
    if (!HasDisplayEnvironment()) {
        if (error) *error = "DISPLAY is not set";
        return false;
    }
    Display *display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        if (error) *error = "cannot open X11 display";
        return false;
    }
    int bytes = 0;
    char *buffer = XFetchBuffer(display, &bytes, 0);
    if (buffer != nullptr && bytes > 0) {
        text.assign(buffer, buffer + bytes);
    }
    if (buffer != nullptr) {
        XFree(buffer);
    }
    XCloseDisplay(display);
    return true;
#else
    text.clear();
    if (error) *error = "X11 clipboard backend was not built";
    return false;
#endif
}

} // namespace linuxclipboard
} // namespace winvnc
} // namespace uvnc
