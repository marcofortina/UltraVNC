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

#if defined(UVNC_HAVE_X11)
#include <X11/Xlib.h>
#include <mutex>
#endif

namespace uvnc {
namespace winvnc {
namespace linuxclipboard {

class X11ClipboardBackend : public portable::RfbClipboardSink, public portable::RfbClipboardSource {
public:
    X11ClipboardBackend();
    ~X11ClipboardBackend() override;

    bool SetText(const std::string& text, std::string *error = nullptr) override;
    bool GetText(std::string& text, std::string *error = nullptr) const override;

    static bool RuntimeAvailable(std::string *reason = nullptr);

private:
#if defined(UVNC_HAVE_X11)
    bool EnsureOwnerDisplay(std::string *error) const;
    void PumpSelectionRequests() const;
    bool FetchExternalSelectionText(std::string& text, std::string *error) const;

    mutable std::mutex mutex_;
    mutable Display *display_;
    mutable Window window_;
    mutable Atom clipboardAtom_;
    mutable Atom targetsAtom_;
    mutable Atom utf8StringAtom_;
    mutable Atom textAtom_;
    mutable Atom selectionPropertyAtom_;
    mutable Atom incrAtom_;
    mutable std::string ownedText_;
#endif
};

} // namespace linuxclipboard
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_CLIPBOARD_BACKEND_H
