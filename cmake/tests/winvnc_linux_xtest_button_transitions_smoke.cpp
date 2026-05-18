// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxXTestInput.h"

#include <cassert>
#include <vector>

using namespace uvnc::winvnc::linuxinput;

int main()
{
    std::vector<ButtonTransition> transitions = XTestInputBackend::ButtonTransitions(0, 0);
    assert(transitions.empty());

    transitions = XTestInputBackend::ButtonTransitions(0, 3);
    assert(transitions.size() == 2);
    assert(transitions[0].button == 1 && transitions[0].down);
    assert(transitions[1].button == 2 && transitions[1].down);

    transitions = XTestInputBackend::ButtonTransitions(3, 2);
    assert(transitions.size() == 1);
    assert(transitions[0].button == 1 && !transitions[0].down);

    transitions = XTestInputBackend::ButtonTransitions(2, 0);
    assert(transitions.size() == 1);
    assert(transitions[0].button == 2 && !transitions[0].down);
    return 0;
}
