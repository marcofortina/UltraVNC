// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxX11FramebufferSource.h"

#include "vncPortableFramebufferDiff.h"

#include <cstring>
#if defined(UVNC_HAVE_X11_XSHM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if defined(UVNC_HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if defined(UVNC_HAVE_X11_XSHM)
#include <X11/extensions/XShm.h>
#endif
#if defined(UVNC_HAVE_X11_XDAMAGE)
#include <X11/extensions/Xdamage.h>
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

class ScopedXErrorTrap {
public:
    explicit ScopedXErrorTrap(Display *display)
        : display_(display),
          previous_(XSetErrorHandler(Trap))
    {
        lastErrorCode_ = 0;
    }

    ~ScopedXErrorTrap()
    {
        if (display_ != nullptr) {
            XSync(display_, False);
        }
        XSetErrorHandler(previous_);
    }

    bool SyncAndOk()
    {
        if (display_ != nullptr) {
            XSync(display_, False);
        }
        return lastErrorCode_ == 0;
    }

private:
    static int Trap(Display *, XErrorEvent *event)
    {
        lastErrorCode_ = event != nullptr ? event->error_code : 1;
        return 0;
    }

    Display *display_;
    XErrorHandler previous_;
    static int lastErrorCode_;
};

int ScopedXErrorTrap::lastErrorCode_ = 0;

#endif // defined(UVNC_HAVE_X11)

bool CopyImageToFramebuffer(const XImage *image, unsigned int width, unsigned int height, portable::Framebuffer& destination, rfb::Region2D& changed)
{
    const rfbPixelFormat capturedFormat = PixelFormatFromImage(image);
    if (!destination.Reset(width, height, capturedFormat)) {
        changed.clear();
        return false;
    }

    const unsigned int rowBytes = width * destination.BytesPerPixel();
    for (unsigned int y = 0; y < height; ++y) {
        const char *src = image->data + y * image->bytes_per_line;
        std::memcpy(destination.PixelAt(0, y), src, rowBytes);
    }
    changed.reset(destination.Bounds());
    return true;
}

#if defined(UVNC_HAVE_X11_XSHM)

bool SnapshotViaXShm(Display *display, Drawable root, unsigned int width, unsigned int height, portable::Framebuffer& destination, rfb::Region2D& changed, rfbPixelFormat& format)
{
    XShmSegmentInfo shminfo;
    std::memset(&shminfo, 0, sizeof(shminfo));

    XImage *image = XShmCreateImage(display,
                                    DefaultVisual(display, DefaultScreen(display)),
                                    static_cast<unsigned int>(DefaultDepth(display, DefaultScreen(display))),
                                    ZPixmap,
                                    nullptr,
                                    &shminfo,
                                    width,
                                    height);
    if (image == nullptr) {
        return false;
    }

    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0600);
    if (shminfo.shmid < 0) {
        XDestroyImage(image);
        return false;
    }

    shminfo.shmaddr = static_cast<char *>(shmat(shminfo.shmid, nullptr, 0));
    if (shminfo.shmaddr == reinterpret_cast<char *>(-1)) {
        shmctl(shminfo.shmid, IPC_RMID, nullptr);
        XDestroyImage(image);
        return false;
    }

    shminfo.readOnly = False;
    image->data = shminfo.shmaddr;

    bool attached = false;
    {
        ScopedXErrorTrap trap(display);
        attached = XShmAttach(display, &shminfo) != 0 && trap.SyncAndOk();
    }

    bool ok = attached;
    if (ok) {
        {
            ScopedXErrorTrap trap(display);
            ok = XShmGetImage(display, root, image, 0, 0, AllPlanes) != 0 && trap.SyncAndOk();
        }
        if (ok) {
            ok = CopyImageToFramebuffer(image, width, height, destination, changed);
            if (ok) {
                format = PixelFormatFromImage(image);
            }
        }
        {
            ScopedXErrorTrap trap(display);
            XShmDetach(display, &shminfo);
            trap.SyncAndOk();
        }
    }

    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, nullptr);
    XDestroyImage(image);
    return ok;
}

#endif // defined(UVNC_HAVE_X11_XSHM)

} // namespace

X11DesktopSource::X11DesktopSource(const std::string& displayName)
    : displayName_(displayName),
      width_(0),
      height_(0),
      format_(),
      display_(nullptr),
      root_(0),
      screen_(0),
      initialized_(false),
      damageAvailable_(false),
      damage_(0),
      damageEventBase_(0),
      damageErrorBase_(0),
      lastError_()
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
      initialized_(false),
      damageAvailable_(false),
      damage_(0),
      damageEventBase_(0),
      damageErrorBase_(0),
      lastError_()
{
}

X11DesktopSource::~X11DesktopSource()
{
#if defined(UVNC_HAVE_X11)
    if (display_ != nullptr) {
#if defined(UVNC_HAVE_X11_XDAMAGE)
        if (damageAvailable_ && damage_ != 0) {
            XDamageDestroy(static_cast<Display *>(display_), static_cast<Damage>(damage_));
            damage_ = 0;
        }
#endif
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
    std::string initError;
    if (!Initialize(&initError)) {
        lastError_ = initError.empty() ? UnavailableReason() : initError;
        destination.Clear();
        changed.clear();
        return false;
    }

    Display *display = static_cast<Display *>(display_);
    rfb::Region2D damageRegion = DrainDamageRegion();
#if defined(UVNC_HAVE_X11_XSHM)
    if (IsXShmRuntimeAvailable(displayName_) &&
        SnapshotViaXShm(display, static_cast<Drawable>(root_), width_, height_, destination, changed, format_)) {
        RefineChangedRegion(destination, changed);
        if (!damageRegion.is_empty()) {
            changed = changed.intersect(damageRegion);
        }
        return true;
    }
#endif

    XImage *image = nullptr;
    {
        ScopedXErrorTrap trap(display);
        image = XGetImage(display,
                          static_cast<Drawable>(root_),
                          0,
                          0,
                          width_,
                          height_,
                          AllPlanes,
                          ZPixmap);
        if (!trap.SyncAndOk()) {
            if (image != nullptr) {
                XDestroyImage(image);
            }
            image = nullptr;
        }
    }
    if (image == nullptr) {
        lastError_ = "XGetImage failed for the X11 root window";
        destination.Clear();
        changed.clear();
        return false;
    }

    bool ok = CopyImageToFramebuffer(image, width_, height_, destination, changed);
    if (ok) {
        format_ = PixelFormatFromImage(image);
        RefineChangedRegion(destination, changed);
        if (!damageRegion.is_empty()) {
            changed = changed.intersect(damageRegion);
        }
        lastError_.clear();
    } else {
        lastError_ = "failed to copy X11 image into the portable framebuffer";
    }

    XDestroyImage(image);
    return ok;
#else
    lastError_ = UnavailableReason();
    destination.Clear();
    changed.clear();
    return false;
#endif
}


void X11DesktopSource::RefineChangedRegion(portable::Framebuffer& destination, rfb::Region2D& changed)
{
    if (!previousFrame_.Empty() && portable::FramebufferDiff::Compatible(previousFrame_, destination)) {
        changed = portable::FramebufferDiff::FindChangedRows(previousFrame_, destination);
    } else {
        changed.reset(destination.Bounds());
    }
    previousFrame_ = destination;
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


bool X11DesktopSource::IsXDamageBuildAvailable()
{
#if defined(UVNC_HAVE_X11_XDAMAGE)
    return true;
#else
    return false;
#endif
}

bool X11DesktopSource::IsXDamageRuntimeAvailable(const std::string& displayName)
{
#if defined(UVNC_HAVE_X11_XDAMAGE)
    Display *display = OpenDisplay(displayName);
    if (display == nullptr) {
        return false;
    }
    int eventBase = 0;
    int errorBase = 0;
    const bool available = XDamageQueryExtension(display, &eventBase, &errorBase) != 0;
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


rfb::Region2D X11DesktopSource::DrainDamageRegion()
{
    rfb::Region2D region;
#if defined(UVNC_HAVE_X11_XDAMAGE)
    if (!damageAvailable_ || display_ == nullptr) {
        return region;
    }
    Display *display = static_cast<Display *>(display_);
    while (XPending(display) > 0) {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == damageEventBase_ + XDamageNotify) {
            XDamageNotifyEvent *damageEvent = reinterpret_cast<XDamageNotifyEvent *>(&event);
            const rfb::Rect rect(damageEvent->area.x,
                                 damageEvent->area.y,
                                 damageEvent->area.x + damageEvent->area.width,
                                 damageEvent->area.y + damageEvent->area.height);
            rfb::Region2D damageRect(rect);
            region.assign_union(damageRect);
        }
    }
    if (damage_ != 0) {
        XDamageSubtract(display, static_cast<Damage>(damage_), None, None);
    }
#endif
    return region;
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
#if defined(UVNC_HAVE_X11_XDAMAGE)
    damageAvailable_ = XDamageQueryExtension(display, &damageEventBase_, &damageErrorBase_) != 0;
    if (damageAvailable_) {
        damage_ = static_cast<unsigned long>(XDamageCreate(display, root, XDamageReportNonEmpty));
        XDamageSubtract(display, static_cast<Damage>(damage_), None, None);
    }
#endif
    initialized_ = true;

    XImage *probe = nullptr;
    {
        ScopedXErrorTrap trap(display);
        probe = XGetImage(display, root, 0, 0, 1, 1, AllPlanes, ZPixmap);
        if (!trap.SyncAndOk()) {
            if (probe != nullptr) {
                XDestroyImage(probe);
            }
            probe = nullptr;
        }
    }
    if (probe != nullptr) {
        format_ = PixelFormatFromImage(probe);
        XDestroyImage(probe);
    }
    lastError_.clear();
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
