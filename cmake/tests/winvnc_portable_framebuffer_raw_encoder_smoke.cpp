// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableFramebuffer.h"
#include "vncencoder.h"

#include <cstring>
#include <vector>

int main()
{
    const rfbPixelFormat format = winvnc_test_true_colour_32();
    uvnc::winvnc::portable::Framebuffer framebuffer(2, 2, format);
    winvnc_test_expect(!framebuffer.Empty(), "framebuffer allocation failed");

    for (std::size_t i = 0; i < framebuffer.SizeBytes(); ++i) {
        framebuffer.Data()[i] = static_cast<BYTE>(i + 1);
    }

    vncEncoder encoder;
    rfbPixelFormat remoteFormat = framebuffer.Format();
    rfbPixelFormat localFormat = framebuffer.Format();
    winvnc_test_expect(!encoder.SetLocalFormat(localFormat, framebuffer.Width(), framebuffer.Height()), "local format should fail until remote format is known");
    winvnc_test_expect(encoder.SetRemoteFormat(remoteFormat), "remote format setup failed");
    winvnc_test_expect(encoder.SetLocalFormat(localFormat, framebuffer.Width(), framebuffer.Height()), "local format setup failed");

    std::vector<BYTE> encoded(encoder.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()));
    const UINT encodedSize = encoder.EncodeRect(framebuffer.Data(), encoded.data(), framebuffer.Bounds());

    winvnc_test_expect(encodedSize == sz_rfbFramebufferUpdateRectHeader + framebuffer.SizeBytes(), "unexpected raw framebuffer encoded size");
    winvnc_test_expect(std::memcmp(encoded.data() + sz_rfbFramebufferUpdateRectHeader, framebuffer.Data(), framebuffer.SizeBytes()) == 0,
                       "encoded framebuffer payload mismatch");

    return 0;
}
