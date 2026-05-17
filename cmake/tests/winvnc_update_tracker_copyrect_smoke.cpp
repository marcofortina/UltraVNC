// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#include "rfbUpdateTracker.h"

namespace {

bool testCopyRectDisabledBecomesChanged()
{
    rfb::SimpleUpdateTracker tracker(false);
    tracker.add_copied(rfb::Region2D(rfb::Rect(10, 10, 20, 20)), rfb::Point(5, 0));

    rfb::UpdateInfo info;
    tracker.flush_update(info, rfb::Region2D(rfb::Rect(0, 0, 100, 100)));

    return info.copied.empty() && info.changed.size() == 1 &&
        info.changed[0].equals(rfb::Rect(10, 10, 20, 20));
}

bool testCopyRectEnabledIsTracked()
{
    rfb::SimpleUpdateTracker tracker(true);
    tracker.add_copied(rfb::Region2D(rfb::Rect(10, 10, 20, 20)), rfb::Point(5, 0));

    rfb::UpdateInfo info;
    tracker.flush_update(info, rfb::Region2D(rfb::Rect(0, 0, 100, 100)));

    return info.changed.empty() && info.copied.size() == 1 &&
        info.copy_delta.equals(rfb::Point(5, 0));
}

}

int main()
{
    if (!testCopyRectDisabledBecomesChanged()) {
        return 1;
    }
    if (!testCopyRectEnabledIsTracked()) {
        return 1;
    }
    return 0;
}
