// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxXTestInput.h"

#if defined(UVNC_HAVE_XTEST)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/Xutil.h>
#endif

namespace uvnc {
namespace winvnc {
namespace linuxinput {
namespace {

#if defined(UVNC_HAVE_XTEST)

Display *OpenDisplay(const std::string& displayName)
{
    return XOpenDisplay(displayName.empty() ? nullptr : displayName.c_str());
}

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

#endif // defined(UVNC_HAVE_XTEST)

} // namespace

XTestInputBackend::XTestInputBackend(const std::string& displayName)
    : displayName_(displayName),
      display_(nullptr),
      buttonMask_(0)
{
}

XTestInputBackend::~XTestInputBackend()
{
#if defined(UVNC_HAVE_XTEST)
    if (display_ != nullptr) {
        XCloseDisplay(static_cast<Display *>(display_));
        display_ = nullptr;
    }
#endif
}

bool XTestInputBackend::InjectKeySym(CARD32 keysym, bool down, std::string *error)
{
#if defined(UVNC_HAVE_XTEST)
    if (!Initialize(error)) {
        return false;
    }
    Display *display = static_cast<Display *>(display_);
    const KeyCode keycode = XKeysymToKeycode(display, static_cast<KeySym>(keysym));
    if (keycode == 0) {
        SetError(error, "XTest cannot translate keysym to keycode");
        return false;
    }
    const bool ok = XTestFakeKeyEvent(display, keycode, down ? True : False, CurrentTime) != 0;
    XSync(display, False);
    if (!ok) {
        SetError(error, "XTest key injection failed");
        return false;
    }
    if (error) error->clear();
    return true;
#else
    (void)keysym;
    (void)down;
    if (error) *error = UnavailableReason();
    return false;
#endif
}

bool XTestInputBackend::InjectPointer(CARD8 buttonMask, unsigned int x, unsigned int y, std::string *error)
{
#if defined(UVNC_HAVE_XTEST)
    if (!Initialize(error)) {
        return false;
    }
    Display *display = static_cast<Display *>(display_);
    const bool moved = XTestFakeMotionEvent(display, DefaultScreen(display), static_cast<int>(x), static_cast<int>(y), CurrentTime) != 0;
    if (!moved) {
        SetError(error, "XTest pointer motion injection failed");
        return false;
    }
    const std::vector<ButtonTransition> transitions = ButtonTransitions(buttonMask_, buttonMask);
    for (std::size_t i = 0; i < transitions.size(); ++i) {
        if (XTestFakeButtonEvent(display, transitions[i].button, transitions[i].down ? True : False, CurrentTime) == 0) {
            SetError(error, "XTest pointer button injection failed");
            return false;
        }
    }
    XSync(display, False);
    buttonMask_ = buttonMask;
    if (error) error->clear();
    return true;
#else
    (void)buttonMask;
    (void)x;
    (void)y;
    if (error) *error = UnavailableReason();
    return false;
#endif
}

bool XTestInputBackend::InjectPointerRelative(int dx, int dy, std::string *error)
{
#if defined(UVNC_HAVE_XTEST)
    if (!Initialize(error)) {
        return false;
    }
    Display *display = static_cast<Display *>(display_);
    Window root = DefaultRootWindow(display);
    Window returnedRoot = 0;
    Window returnedChild = 0;
    int rootX = 0;
    int rootY = 0;
    int winX = 0;
    int winY = 0;
    unsigned int mask = 0;
    if (XQueryPointer(display, root, &returnedRoot, &returnedChild, &rootX, &rootY, &winX, &winY, &mask) == 0) {
        SetError(error, "XTest cannot query current pointer position");
        return false;
    }
    const bool moved = XTestFakeMotionEvent(display, DefaultScreen(display), rootX + dx, rootY + dy, CurrentTime) != 0;
    XSync(display, False);
    if (!moved) {
        SetError(error, "XTest relative pointer motion injection failed");
        return false;
    }
    if (error) error->clear();
    return true;
#else
    (void)dx;
    (void)dy;
    if (error) *error = UnavailableReason();
    return false;
#endif
}

bool XTestInputBackend::InjectButton(unsigned int button, bool down, std::string *error)
{
#if defined(UVNC_HAVE_XTEST)
    if (!Initialize(error)) {
        return false;
    }
    Display *display = static_cast<Display *>(display_);
    const bool ok = XTestFakeButtonEvent(display, button, down ? True : False, CurrentTime) != 0;
    XSync(display, False);
    if (!ok) {
        SetError(error, "XTest button injection failed");
        return false;
    }
    if (error) error->clear();
    return true;
#else
    (void)button;
    (void)down;
    if (error) *error = UnavailableReason();
    return false;
#endif
}

bool XTestInputBackend::IsBuildAvailable()
{
#if defined(UVNC_HAVE_XTEST)
    return true;
#else
    return false;
#endif
}

bool XTestInputBackend::IsAvailable(const std::string& displayName)
{
#if defined(UVNC_HAVE_XTEST)
    Display *display = OpenDisplay(displayName);
    if (display == nullptr) {
        return false;
    }
    int eventBase = 0;
    int errorBase = 0;
    int major = 0;
    int minor = 0;
    const bool available = XTestQueryExtension(display, &eventBase, &errorBase, &major, &minor) != 0;
    XCloseDisplay(display);
    return available;
#else
    (void)displayName;
    return false;
#endif
}

const char *XTestInputBackend::UnavailableReason()
{
#if defined(UVNC_HAVE_XTEST)
    return "XTest input backend is built, but no usable XTest DISPLAY is available";
#else
    return "XTest input backend was not built because XTest development files were not available";
#endif
}

std::vector<ButtonTransition> XTestInputBackend::ButtonTransitions(CARD8 previousMask, CARD8 nextMask)
{
    std::vector<ButtonTransition> transitions;
    for (unsigned int bit = 0; bit < 8; ++bit) {
        const CARD8 mask = static_cast<CARD8>(1U << bit);
        const bool previousDown = (previousMask & mask) != 0;
        const bool nextDown = (nextMask & mask) != 0;
        if (previousDown != nextDown) {
            ButtonTransition transition;
            transition.button = bit + 1;
            transition.down = nextDown;
            transitions.push_back(transition);
        }
    }
    return transitions;
}

bool XTestInputBackend::Initialize(std::string *error)
{
#if defined(UVNC_HAVE_XTEST)
    if (display_ != nullptr) {
        return true;
    }
    Display *display = OpenDisplay(displayName_);
    if (display == nullptr) {
        SetError(error, UnavailableReason());
        return false;
    }
    int eventBase = 0;
    int errorBase = 0;
    int major = 0;
    int minor = 0;
    if (XTestQueryExtension(display, &eventBase, &errorBase, &major, &minor) == 0) {
        XCloseDisplay(display);
        SetError(error, "XTest extension is not available on this DISPLAY");
        return false;
    }
    display_ = display;
    if (error) error->clear();
    return true;
#else
    if (error) *error = UnavailableReason();
    return false;
#endif
}

} // namespace linuxinput
} // namespace winvnc
} // namespace uvnc
