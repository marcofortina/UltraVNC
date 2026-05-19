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
#include <X11/Xatom.h>
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

X11ClipboardBackend::X11ClipboardBackend()
#if defined(UVNC_HAVE_X11)
    : display_(nullptr),
      window_(0),
      clipboardAtom_(None),
      targetsAtom_(None),
      utf8StringAtom_(None),
      textAtom_(None),
      ownedText_()
#endif
{
}

X11ClipboardBackend::~X11ClipboardBackend()
{
#if defined(UVNC_HAVE_X11)
    std::lock_guard<std::mutex> lock(mutex_);
    if (display_ != nullptr) {
        if (window_ != 0) {
            XDestroyWindow(display_, window_);
            window_ = 0;
        }
        XCloseDisplay(display_);
        display_ = nullptr;
    }
#endif
}

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

#if defined(UVNC_HAVE_X11)
bool X11ClipboardBackend::EnsureOwnerDisplay(std::string *error) const
{
    if (!HasDisplayEnvironment()) {
        if (error) *error = "DISPLAY is not set";
        return false;
    }
    if (display_ != nullptr) {
        return true;
    }
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        if (error) *error = "cannot open X11 display";
        return false;
    }
    window_ = XCreateSimpleWindow(display_, DefaultRootWindow(display_), 0, 0, 1, 1, 0, 0, 0);
    clipboardAtom_ = XInternAtom(display_, "CLIPBOARD", False);
    targetsAtom_ = XInternAtom(display_, "TARGETS", False);
    utf8StringAtom_ = XInternAtom(display_, "UTF8_STRING", False);
    textAtom_ = XInternAtom(display_, "TEXT", False);
    return true;
}

void X11ClipboardBackend::PumpSelectionRequests() const
{
    if (display_ == nullptr) {
        return;
    }
    while (XPending(display_) > 0) {
        XEvent event;
        XNextEvent(display_, &event);
        if (event.type != SelectionRequest) {
            continue;
        }
        XSelectionRequestEvent *request = &event.xselectionrequest;
        XSelectionEvent notify;
        notify.type = SelectionNotify;
        notify.display = request->display;
        notify.requestor = request->requestor;
        notify.selection = request->selection;
        notify.target = request->target;
        notify.time = request->time;
        notify.property = None;

        if (request->selection == clipboardAtom_ && request->property != None) {
            if (request->target == targetsAtom_) {
                Atom targets[3] = {targetsAtom_, utf8StringAtom_, XA_STRING};
                XChangeProperty(display_, request->requestor, request->property, XA_ATOM, 32,
                                PropModeReplace, reinterpret_cast<unsigned char *>(targets), 3);
                notify.property = request->property;
            } else if (request->target == utf8StringAtom_ || request->target == XA_STRING || request->target == textAtom_) {
                const Atom propertyType = request->target == XA_STRING ? XA_STRING : utf8StringAtom_;
                XChangeProperty(display_, request->requestor, request->property, propertyType, 8,
                                PropModeReplace,
                                reinterpret_cast<const unsigned char *>(ownedText_.data()),
                                static_cast<int>(ownedText_.size()));
                notify.property = request->property;
            }
        }
        XSendEvent(display_, request->requestor, False, 0, reinterpret_cast<XEvent *>(&notify));
        XFlush(display_);
    }
}
#endif

bool X11ClipboardBackend::SetText(const std::string& text, std::string *error)
{
#if defined(UVNC_HAVE_X11)
    std::lock_guard<std::mutex> lock(mutex_);
    if (!EnsureOwnerDisplay(error)) {
        return false;
    }
    ownedText_ = text;
    XStoreBuffer(display_, text.data(), static_cast<int>(text.size()), 0);
    XSetSelectionOwner(display_, clipboardAtom_, window_, CurrentTime);
    if (XGetSelectionOwner(display_, clipboardAtom_) != window_) {
        if (error) *error = "cannot own X11 CLIPBOARD selection";
        return false;
    }
    PumpSelectionRequests();
    XFlush(display_);
    return true;
#else
    if (error) *error = "X11 clipboard backend was not built";
    return false;
#endif
}

bool X11ClipboardBackend::GetText(std::string& text, std::string *error) const
{
#if defined(UVNC_HAVE_X11)
    std::lock_guard<std::mutex> lock(mutex_);
    text.clear();
    if (!EnsureOwnerDisplay(error)) {
        return false;
    }
    PumpSelectionRequests();
    if (!ownedText_.empty() && XGetSelectionOwner(display_, clipboardAtom_) == window_) {
        text = ownedText_;
        return true;
    }
    int bytes = 0;
    char *buffer = XFetchBuffer(display_, &bytes, 0);
    if (buffer != nullptr && bytes > 0) {
        text.assign(buffer, buffer + bytes);
    }
    if (buffer != nullptr) {
        XFree(buffer);
    }
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
