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

#include <mutex>
#include <string>

struct _XDisplay;
typedef struct _XDisplay Display;

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
    bool EnsureOwnerDisplay(std::string *error) const;
    void PumpSelectionRequests() const;
    bool FetchExternalSelectionText(std::string& text, std::string *error) const;

    mutable std::mutex mutex_;
    mutable Display *display_;
    mutable unsigned long window_;
    mutable unsigned long clipboardAtom_;
    mutable unsigned long targetsAtom_;
    mutable unsigned long utf8StringAtom_;
    mutable unsigned long textAtom_;
    mutable unsigned long selectionPropertyAtom_;
    mutable unsigned long incrAtom_;
    mutable std::string ownedText_;
};

} // namespace linuxclipboard
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_CLIPBOARD_BACKEND_H
