// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

// Portable rfb::Region2D implementation for non-Windows native builds.

#ifndef __RFB_REGION_PORTABLE_INCLUDED__
#define __RFB_REGION_PORTABLE_INCLUDED__

#include "rfbRect.h"
#include <vector>

namespace rfb {

  class Region {
  public:
    Region();
    Region(int x1, int y1, int x2, int y2);
    Region(const Rect& r);

    Region(const Region& r);
    Region &operator=(const Region& src);

    ~Region();

    void clear();
    void reset(const Rect& r);
    void translate(const rfb::Point& delta);
    void setOrderedRects(const std::vector<Rect>& rects);

    void assign_intersect(const Region& r);
    void assign_union(const Region& r);
    void assign_subtract(const Region& r);

    Region intersect(const Region& r) const;
    Region union_(const Region& r) const;
    Region subtract(const Region& r) const;

    bool equals(const Region& b) const;
    bool is_empty() const;

    bool get_rects(std::vector<Rect>& rects, bool left2right=true,
                   bool topdown=true) const;
    Rect get_bounding_rect() const;
    int Numrects();

    void debug_print(const char *prefix) const;

  private:
    std::vector<Rect> rects;
  };

  typedef Region Region2D;

}

#endif // __RFB_REGION_PORTABLE_INCLUDED__
