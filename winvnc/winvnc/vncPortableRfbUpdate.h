// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_PORTABLE_RFB_UPDATE_H
#define UVNC_WINVNC_PORTABLE_RFB_UPDATE_H

#include "vncPortableFramebuffer.h"
#include "vncPortableRfbMessages.h"

#include <vector>

namespace uvnc {
namespace winvnc {
namespace portable {

std::vector<CARD8> EmptyFramebufferUpdateBytes();
std::vector<CARD8> RawFramebufferUpdateBytes(const Framebuffer& framebuffer,
                                             const FramebufferUpdateRequest& request);
std::vector<CARD8> EncodedFramebufferUpdateBytes(const Framebuffer& framebuffer,
                                                 const FramebufferUpdateRequest& request,
                                                 const rfbPixelFormat& remoteFormat,
                                                 const std::vector<CARD32>& preferredEncodings);
std::vector<CARD8> PointerPositionUpdateBytes(unsigned int x, unsigned int y);

} // namespace portable
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_PORTABLE_RFB_UPDATE_H
