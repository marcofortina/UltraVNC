// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerSecurity.h"

#include <cassert>
#include <vector>

using namespace uvnc::vncviewer::portable;

int main()
{
    {
        std::vector<CARD8> types(1, rfbNoAuth);
        ViewerSecurityDecision decision = SelectViewerSecurityType(types, false);
        assert(decision.selection == ViewerSecuritySelection::NoAuth);
        assert(decision.wireType == rfbNoAuth);
    }
    {
        std::vector<CARD8> types;
        types.push_back(rfbNoAuth);
        types.push_back(rfbVncAuth);
        ViewerSecurityDecision decision = SelectViewerSecurityType(types, true);
        assert(decision.selection == ViewerSecuritySelection::VncAuth);
        assert(decision.wireType == rfbVncAuth);
    }
    {
        std::vector<CARD8> types(1, rfbVncAuth);
        ViewerSecurityDecision decision = SelectViewerSecurityType(types, false);
        assert(decision.selection == ViewerSecuritySelection::Unsupported);
        assert(decision.error.find("password") != std::string::npos);
    }
    {
        std::vector<CARD8> types(1, rfbVeNCypt);
        ViewerSecurityDecision decision = SelectViewerSecurityType(types, true);
        assert(decision.selection == ViewerSecuritySelection::Unsupported);
        assert(decision.error.find("VeNCrypt") != std::string::npos);
    }
    {
        std::vector<CARD8> types(1, rfbMSLogon);
        ViewerSecurityDecision decision = SelectViewerSecurityType(types, true);
        assert(decision.selection == ViewerSecuritySelection::Unsupported);
        assert(decision.error.find("mslogon") != std::string::npos);
    }
    return 0;
}
