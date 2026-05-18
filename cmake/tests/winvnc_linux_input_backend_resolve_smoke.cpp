// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxInputBackend.h"
#include "vncLinuxXTestInput.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxinput;

int main()
{
    InputBackend resolved = InputBackend::XTest;
    std::string error;
    assert(ResolveInputBackend(InputBackend::Auto, resolved, &error));
    assert(error.empty());
    assert((resolved == InputBackend::None) || (resolved == InputBackend::XTest));

    assert(ResolveInputBackend(InputBackend::None, resolved, &error));
    assert(resolved == InputBackend::None);
    assert(error.empty());

    if (XTestInputBackend::IsAvailable()) {
        assert(ResolveInputBackend(InputBackend::XTest, resolved, &error));
        assert(resolved == InputBackend::XTest);
        assert(error.empty());
    } else {
        assert(!ResolveInputBackend(InputBackend::XTest, resolved, &error));
        assert(error.find("xtest") != std::string::npos);
    }
    return 0;
}
