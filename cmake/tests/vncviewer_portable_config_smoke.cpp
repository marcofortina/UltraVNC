// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerConfig.h"

#include "rfb.h"

#include <cassert>
#include <string>

using uvnc::vncviewer::portable::ViewerConfig;

int main()
{
    ViewerConfig config;
    std::string error;

    assert(config.Host() == "127.0.0.1");
    assert(config.Port() == 5900);
    assert(config.Shared());
    assert(!config.RequestUpdate());
    assert(!config.ViewOnly());
    assert(config.Validate(&error));
    assert(error.empty());
    assert(config.Encodings().size() == 14);
    assert(config.Encodings()[9] == rfbEncodingRichCursor);
    assert(config.Encodings()[10] == rfbEncodingXCursor);
    assert(config.Encodings()[11] == rfbEncodingPointerPos);
    assert(config.Encodings()[12] == rfbEncodingLastRect);
    assert(config.Encodings()[13] == rfbEncodingExtendedClipboard);

    config.SetHost("");
    assert(!config.Validate(&error));
    assert(error == "viewer host must not be empty");

    config.SetHost("127.0.0.1");
    config.SetPort(0);
    assert(!config.Validate(&error));
    assert(error == "viewer port must not be zero");

    config.SetPort(5901);
    config.SetRequestUpdate(true);
    config.SetViewOnly(true);
    assert(config.Validate(&error));
    assert(config.RequestUpdate());
    assert(config.ViewOnly());
    return 0;
}
