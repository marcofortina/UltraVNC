// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11CursorSource.h"

#include <cstdlib>

#if defined(UVNC_HAVE_X11_XFIXES)
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>

// X11/X.h defines a CursorShape macro for QueryBestSize.
// Keep that legacy X11 token out of the portable cursor shape API.
#ifdef CursorShape
#undef CursorShape
#endif
#endif

namespace uvnc {
namespace winvnc {
namespace linuxfb {
namespace {

#if defined(UVNC_HAVE_X11_XFIXES)
Display *OpenDisplay(const std::string& displayName)
{
    return XOpenDisplay(displayName.empty() ? nullptr : displayName.c_str());
}
#endif

bool HasDisplayEnvironment()
{
    const char *display = std::getenv("DISPLAY");
    return display != nullptr && display[0] != '\0';
}

} // namespace

X11CursorSource::X11CursorSource(const std::string& displayName)
    : displayName_(displayName), display_(nullptr)
{
}

X11CursorSource::~X11CursorSource()
{
#if defined(UVNC_HAVE_X11_XFIXES)
    if (display_ != nullptr) {
        XCloseDisplay(static_cast<Display *>(display_));
        display_ = nullptr;
    }
#endif
}

bool X11CursorSource::IsBuildAvailable()
{
#if defined(UVNC_HAVE_X11_XFIXES)
    return true;
#else
    return false;
#endif
}

const char *X11CursorSource::UnavailableReason()
{
#if defined(UVNC_HAVE_X11_XFIXES)
    return "X11 XFixes cursor backend is built, but no usable X11 DISPLAY is available";
#else
    return "X11 XFixes cursor backend was not built because Xfixes development files were not available";
#endif
}

bool X11CursorSource::IsRuntimeAvailable(const std::string& displayName, std::string *reason)
{
#if defined(UVNC_HAVE_X11_XFIXES)
    if (!HasDisplayEnvironment() && displayName.empty()) {
        if (reason) *reason = "DISPLAY is not set";
        return false;
    }
    Display *display = OpenDisplay(displayName);
    if (display == nullptr) {
        if (reason) *reason = "cannot open X11 display";
        return false;
    }
    int eventBase = 0;
    int errorBase = 0;
    const bool available = XFixesQueryExtension(display, &eventBase, &errorBase) != 0;
    XCloseDisplay(display);
    if (!available && reason) *reason = "XFixes extension is not available on the X11 display";
    return available;
#else
    (void)displayName;
    if (reason) *reason = UnavailableReason();
    return false;
#endif
}

bool X11CursorSource::EnsureDisplay(std::string *error) const
{
#if defined(UVNC_HAVE_X11_XFIXES)
    if (display_ != nullptr) return true;
    if (!HasDisplayEnvironment() && displayName_.empty()) {
        if (error) *error = "DISPLAY is not set";
        return false;
    }
    Display *display = OpenDisplay(displayName_);
    if (display == nullptr) {
        if (error) *error = "cannot open X11 display";
        return false;
    }
    int eventBase = 0;
    int errorBase = 0;
    if (XFixesQueryExtension(display, &eventBase, &errorBase) == 0) {
        XCloseDisplay(display);
        if (error) *error = "XFixes extension is not available on the X11 display";
        return false;
    }
    display_ = display;
    return true;
#else
    if (error) *error = UnavailableReason();
    return false;
#endif
}

bool X11CursorSource::GetCursorShape(portable::CursorShape& shape, std::string *error) const
{
#if defined(UVNC_HAVE_X11_XFIXES)
    shape = portable::EmptyCursorShape();
    if (!EnsureDisplay(error)) return false;
    Display *display = static_cast<Display *>(display_);
    XFixesCursorImage *image = XFixesGetCursorImage(display);
    if (image == nullptr) {
        if (error) *error = "XFixesGetCursorImage failed";
        return false;
    }
    shape.width = static_cast<unsigned int>(image->width);
    shape.height = static_cast<unsigned int>(image->height);
    shape.hotspotX = image->xhot < image->width ? static_cast<unsigned int>(image->xhot) : 0;
    shape.hotspotY = image->yhot < image->height ? static_cast<unsigned int>(image->yhot) : 0;
    shape.bgra.assign(static_cast<std::size_t>(shape.width) * shape.height * 4, 0);
    for (unsigned int y = 0; y < shape.height; ++y) {
        for (unsigned int x = 0; x < shape.width; ++x) {
            const unsigned long argb = image->pixels[static_cast<std::size_t>(y) * shape.width + x];
            const std::size_t out = (static_cast<std::size_t>(y) * shape.width + x) * 4;
            shape.bgra[out + 0] = static_cast<CARD8>(argb & 0xffUL);
            shape.bgra[out + 1] = static_cast<CARD8>((argb >> 8) & 0xffUL);
            shape.bgra[out + 2] = static_cast<CARD8>((argb >> 16) & 0xffUL);
            shape.bgra[out + 3] = static_cast<CARD8>((argb >> 24) & 0xffUL);
        }
    }
    XFree(image);
    if (!shape.Valid()) {
        shape = portable::EmptyCursorShape();
        if (error) *error = "XFixes returned an invalid cursor image";
        return false;
    }
    if (error) error->clear();
    return true;
#else
    shape = portable::EmptyCursorShape();
    if (error) *error = UnavailableReason();
    return false;
#endif
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
