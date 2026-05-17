// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
#define UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H

#include "rfb.h"

namespace uvnc {
namespace winvnc {
namespace portable {

struct FramebufferUpdateRequest {
    bool incremental;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

bool DecodeFramebufferUpdateRequest(const rfbFramebufferUpdateRequestMsg& message,
                                    FramebufferUpdateRequest& out);
rfbFramebufferUpdateRequestMsg EncodeFramebufferUpdateRequest(const FramebufferUpdateRequest& request);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_MESSAGES_H
