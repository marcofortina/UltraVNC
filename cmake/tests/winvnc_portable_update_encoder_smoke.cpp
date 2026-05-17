// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "winvnc_portable_test_utils.h"
#include "vncPortableUpdateEncoder.h"

#include <cstring>
#include <vector>

int main()
{
    using uvnc::winvnc::portable::Framebuffer;
    using uvnc::winvnc::portable::UpdateEncoder;

    Framebuffer framebuffer(2, 2, winvnc_test_true_colour_32());
    for (std::size_t i = 0; i < framebuffer.SizeBytes(); ++i) {
        framebuffer.Data()[i] = static_cast<BYTE>(i + 3);
    }

    UpdateEncoder encoder;
    std::vector<BYTE> encoded;

    winvnc_test_expect(!encoder.EncodeRawRect(framebuffer, framebuffer.Bounds(), encoded), "encode should fail before initialization");
    winvnc_test_expect(encoder.Initialize(framebuffer.Format(), framebuffer.Width(), framebuffer.Height()), "update encoder initialization failed");
    winvnc_test_expect(encoder.EncodeRawRect(framebuffer, framebuffer.Bounds(), encoded), "raw update encode failed");
    winvnc_test_expect(encoded.size() == sz_rfbFramebufferUpdateRectHeader + framebuffer.SizeBytes(), "unexpected raw update size");
    winvnc_test_expect(std::memcmp(encoded.data() + sz_rfbFramebufferUpdateRectHeader, framebuffer.Data(), framebuffer.SizeBytes()) == 0,
                       "raw update payload mismatch");
    winvnc_test_expect(!encoder.EncodeRawRect(framebuffer, winvnc_test_rect(0, 0, 3, 3), encoded), "out-of-bounds raw update succeeded");

    return 0;
}
