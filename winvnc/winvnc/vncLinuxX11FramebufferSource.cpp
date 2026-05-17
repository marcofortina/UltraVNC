// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"

#include <cstring>

#if defined(UVNC_HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if defined(UVNC_HAVE_X11_XSHM)
#include <X11/extensions/XShm.h>
#endif
#endif

namespace uvnc {
namespace winvnc {
namespace linuxfb {
namespace {

#if defined(UVNC_HAVE_X11)

unsigned int MaskShift(unsigned long mask)
{
    unsigned int shift = 0;
    while (mask != 0 && (mask & 1UL) == 0) {
        mask >>= 1;
        shift += 1;
    }
    return shift;
}

CARD16 MaskMax(unsigned long mask)
{
    const unsigned int shift = MaskShift(mask);
    return static_cast<CARD16>(mask >> shift);
}

rfbPixelFormat PixelFormatFromImage(const XImage *image)
{
    rfbPixelFormat format;
    std::memset(&format, 0, sizeof(format));
    format.bitsPerPixel = static_cast<CARD8>(image->bits_per_pixel);
    format.depth = static_cast<CARD8>(image->depth);
    format.bigEndian = image->byte_order == MSBFirst ? 1 : 0;
    format.trueColour = 1;
    format.redMax = MaskMax(image->red_mask);
    format.greenMax = MaskMax(image->green_mask);
    format.blueMax = MaskMax(image->blue_mask);
    format.redShift = static_cast<CARD8>(MaskShift(image->red_mask));
    format.greenShift = static_cast<CARD8>(MaskShift(image->green_mask));
    format.blueShift = static_cast<CARD8>(MaskShift(image->blue_mask));
    return format;
}

Display *OpenDisplay(const std::string& displayName)
{
    return XOpenDisplay(displayName.empty() ? nullptr : displayName.c_str());
}

#endif // defined(UVNC_HAVE_X11)

} // namespace

X11DesktopSource::X11DesktopSource(const std::string& displayName)
    : displayName_(displayName),
      width_(0),
      height_(0),
      format_(),
      display_(nullptr),
      root_(0),
      screen_(0),
      initialized_(false)
{
    std::memset(&format_, 0, sizeof(format_));
}

X11DesktopSource::X11DesktopSource(unsigned int width, unsigned int height, const rfbPixelFormat& format)
    : displayName_(),
      width_(width),
      height_(height),
      format_(format),
      display_(nullptr),
      root_(0),
      screen_(0),
      initialized_(false)
{
}

X11DesktopSource::~X11DesktopSource()
{
#if defined(UVNC_HAVE_X11)
    if (display_ != nullptr) {
        XCloseDisplay(static_cast<Display *>(display_));
        display_ = nullptr;
    }
#endif
}

rfb::Rect X11DesktopSource::Size() const
{
    return rfb::Rect(0, 0, static_cast<int>(width_), static_cast<int>(height_));
}

rfbPixelFormat X11DesktopSource::Format() const
{
    return format_;
}

bool X11DesktopSource::Snapshot(portable::Framebuffer& destination, rfb::Region2D& changed)
{
#if defined(UVNC_HAVE_X11)
    if (!Initialize()) {
        destination.Clear();
        changed.clear();
        return false;
    }

    Display *display = static_cast<Display *>(display_);
    XImage *image = XGetImage(display,
                              static_cast<Drawable>(root_),
                              0,
                              0,
                              width_,
                              height_,
                              AllPlanes,
                              ZPixmap);
    if (image == nullptr) {
        destination.Clear();
        changed.clear();
        return false;
    }

    const rfbPixelFormat capturedFormat = PixelFormatFromImage(image);
    bool ok = destination.Reset(width_, height_, capturedFormat);
    if (ok) {
        const unsigned int rowBytes = width_ * destination.BytesPerPixel();
        for (unsigned int y = 0; y < height_; ++y) {
            const char *src = image->data + y * image->bytes_per_line;
            std::memcpy(destination.PixelAt(0, y), src, rowBytes);
        }
        changed.reset(destination.Bounds());
        format_ = capturedFormat;
    } else {
        changed.clear();
    }

    XDestroyImage(image);
    return ok;
#else
    destination.Clear();
    changed.clear();
    return false;
#endif
}

bool X11DesktopSource::IsBuildAvailable()
{
#if defined(UVNC_HAVE_X11)
    return true;
#else
    return false;
#endif
}

bool X11DesktopSource::IsAvailable(const std::string& displayName)
{
#if defined(UVNC_HAVE_X11)
    Display *display = OpenDisplay(displayName);
    if (display == nullptr) {
        return false;
    }
    XCloseDisplay(display);
    return true;
#else
    (void)displayName;
    return false;
#endif
}

bool X11DesktopSource::IsXShmBuildAvailable()
{
#if defined(UVNC_HAVE_X11_XSHM)
    return true;
#else
    return false;
#endif
}

bool X11DesktopSource::IsXShmRuntimeAvailable(const std::string& displayName)
{
#if defined(UVNC_HAVE_X11_XSHM)
    Display *display = OpenDisplay(displayName);
    if (display == nullptr) {
        return false;
    }
    const bool available = XShmQueryExtension(display) != 0;
    XCloseDisplay(display);
    return available;
#else
    (void)displayName;
    return false;
#endif
}

const char *X11DesktopSource::UnavailableReason()
{
#if defined(UVNC_HAVE_X11)
    return "X11 capture backend is built, but no usable X11 DISPLAY is available";
#else
    return "X11 capture backend was not built because X11 development files were not available";
#endif
}

bool X11DesktopSource::Initialize(std::string *error)
{
#if defined(UVNC_HAVE_X11)
    if (initialized_) {
        return true;
    }

    Display *display = OpenDisplay(displayName_);
    if (display == nullptr) {
        if (error) *error = UnavailableReason();
        return false;
    }

    const int screen = DefaultScreen(display);
    const Window root = RootWindow(display, screen);
    XWindowAttributes attributes;
    if (XGetWindowAttributes(display, root, &attributes) == 0) {
        XCloseDisplay(display);
        if (error) *error = "cannot query X11 root window attributes";
        return false;
    }

    width_ = static_cast<unsigned int>(attributes.width);
    height_ = static_cast<unsigned int>(attributes.height);
    display_ = display;
    root_ = static_cast<unsigned long>(root);
    screen_ = screen;
    initialized_ = true;

    XImage *probe = XGetImage(display, root, 0, 0, 1, 1, AllPlanes, ZPixmap);
    if (probe != nullptr) {
        format_ = PixelFormatFromImage(probe);
        XDestroyImage(probe);
    }
    if (error) error->clear();
    return true;
#else
    if (error) *error = UnavailableReason();
    return false;
#endif
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
