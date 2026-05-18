// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxInputBackend.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::linuxinput;

int main()
{
    InputBackend backend = InputBackend::None;
    assert(ParseInputBackendName("auto", backend) && backend == InputBackend::Auto);
    assert(ParseInputBackendName("none", backend) && backend == InputBackend::None);
    assert(ParseInputBackendName("xtest", backend) && backend == InputBackend::XTest);
    assert(!ParseInputBackendName("pipewire", backend));

    assert(std::string(InputBackendName(InputBackend::Auto)) == "auto");
    assert(std::string(InputBackendName(InputBackend::None)) == "none");
    assert(std::string(InputBackendName(InputBackend::XTest)) == "xtest");

    assert(std::string(InputBackendDescription(InputBackend::XTest)).find("XTest") != std::string::npos);
    assert(IsInputBackendRuntimeAvailable(InputBackend::Auto));
    assert(IsInputBackendRuntimeAvailable(InputBackend::None));
    return 0;
}
