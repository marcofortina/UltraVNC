// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxXTestInput.h"

#include <cassert>

using namespace uvnc::winvnc::linuxinput;

int main()
{
    KeyModifierPlan plan = XTestInputBackend::PortableModifierPlan('A');
    assert(plan.keysym == 'a');
    assert(plan.shift);
    assert(!plan.altGr);

    plan = XTestInputBackend::PortableModifierPlan('!');
    assert(plan.keysym == '1');
    assert(plan.shift);

    plan = XTestInputBackend::PortableModifierPlan('?');
    assert(plan.keysym == '/');
    assert(plan.shift);

    plan = XTestInputBackend::PortableModifierPlan('z');
    assert(plan.keysym == 'z');
    assert(!plan.shift);
    return 0;
}
