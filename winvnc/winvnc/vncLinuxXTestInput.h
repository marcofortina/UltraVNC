// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_LINUX_XTEST_INPUT_H
#define UVNC_WINVNC_LINUX_XTEST_INPUT_H

#include "rfb.h"

#include <string>
#include <vector>

namespace uvnc {
namespace winvnc {
namespace linuxinput {

struct ButtonTransition {
    unsigned int button;
    bool down;
};

class XTestInputBackend {
public:
    explicit XTestInputBackend(const std::string& displayName = std::string());
    ~XTestInputBackend();

    bool InjectKeySym(CARD32 keysym, bool down, std::string *error = nullptr);
    bool InjectPointer(CARD8 buttonMask, unsigned int x, unsigned int y, std::string *error = nullptr);

    static bool IsBuildAvailable();
    static bool IsAvailable(const std::string& displayName = std::string());
    static const char *UnavailableReason();
    static std::vector<ButtonTransition> ButtonTransitions(CARD8 previousMask, CARD8 nextMask);

private:
    bool Initialize(std::string *error = nullptr);

    std::string displayName_;
    void *display_;
    unsigned char buttonMask_;
};

} // namespace linuxinput
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_LINUX_XTEST_INPUT_H
