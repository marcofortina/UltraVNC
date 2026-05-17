// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#ifndef UVNC_WINVNC_VSOCKET_PORTABLE_H
#define UVNC_WINVNC_VSOCKET_PORTABLE_H

#include <cstddef>

class VSocket {
public:
    virtual ~VSocket() = default;

    virtual void SendExactQueue(char *, int) {}
    virtual bool SendExact(const char *, int) { return true; }

    void *m_pIntegratedPluginInterface = nullptr;
};

#endif // UVNC_WINVNC_VSOCKET_PORTABLE_H
