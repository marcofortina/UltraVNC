// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#include "rfbRegion.h"

namespace {

bool testUnionAndBoundingRect()
{
    rfb::Region2D region(rfb::Rect(0, 0, 10, 10));
    region.assign_union(rfb::Region2D(rfb::Rect(20, 20, 30, 30)));

    const rfb::Rect bounds = region.get_bounding_rect();
    return region.Numrects() == 2 && bounds.equals(rfb::Rect(0, 0, 30, 30));
}

bool testIntersectAndSubtract()
{
    const rfb::Region2D region(rfb::Rect(0, 0, 10, 10));
    const rfb::Region2D clip(rfb::Rect(5, 5, 12, 12));
    const rfb::Region2D intersection = region.intersect(clip);
    const rfb::Region2D remainder = region.subtract(intersection);

    return intersection.get_bounding_rect().equals(rfb::Rect(5, 5, 10, 10)) &&
        !remainder.is_empty();
}

bool testTranslate()
{
    rfb::Region2D region(rfb::Rect(1, 2, 3, 4));
    region.translate(rfb::Point(10, 20));

    return region.get_bounding_rect().equals(rfb::Rect(11, 22, 13, 24));
}

}

int main()
{
    if (!testUnionAndBoundingRect()) {
        return 1;
    }
    if (!testIntersectAndSubtract()) {
        return 1;
    }
    if (!testTranslate()) {
        return 1;
    }
    return 0;
}
