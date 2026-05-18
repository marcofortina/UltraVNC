// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxXTestInput.h"

namespace uvnc {
namespace winvnc {
namespace linuxinput {

XTestInputBackend::XTestInputBackend(const std::string& displayName)
    : displayName_(displayName),
      display_(nullptr),
      buttonMask_(0)
{
}

XTestInputBackend::~XTestInputBackend()
{
}

bool XTestInputBackend::InjectKeySym(CARD32, bool, std::string *error)
{
    if (error) *error = UnavailableReason();
    return false;
}

bool XTestInputBackend::InjectPointer(CARD8, unsigned int, unsigned int, std::string *error)
{
    if (error) *error = UnavailableReason();
    return false;
}

bool XTestInputBackend::IsBuildAvailable()
{
    return false;
}

bool XTestInputBackend::IsAvailable(const std::string& displayName)
{
    (void)displayName;
    return false;
}

const char *XTestInputBackend::UnavailableReason()
{
    return "XTest input backend was not built because XTest development files were not available";
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
    if (error) *error = UnavailableReason();
    return false;
}

} // namespace linuxinput
} // namespace winvnc
} // namespace uvnc
