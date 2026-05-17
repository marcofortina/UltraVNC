// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//

#include "rfbRegion_portable.h"

#include <algorithm>
#include <cstdio>

using namespace rfb;

namespace {

void append_if_not_empty(std::vector<Rect>& rects, const Rect& rect)
{
  if (!rect.is_empty())
    rects.push_back(rect);
}

std::vector<Rect> subtract_rect(const Rect& source, const Rect& cut)
{
  std::vector<Rect> result;
  const Rect overlap = source.intersect(cut);
  if (overlap.is_empty()) {
    result.push_back(source);
    return result;
  }

  append_if_not_empty(result, Rect(source.tl.x, source.tl.y, source.br.x, overlap.tl.y));
  append_if_not_empty(result, Rect(source.tl.x, overlap.br.y, source.br.x, source.br.y));
  append_if_not_empty(result, Rect(source.tl.x, overlap.tl.y, overlap.tl.x, overlap.br.y));
  append_if_not_empty(result, Rect(overlap.br.x, overlap.tl.y, source.br.x, overlap.br.y));
  return result;
}

bool rect_less_topdown_lefttoright(const Rect& a, const Rect& b)
{
  if (a.tl.y != b.tl.y)
    return a.tl.y < b.tl.y;
  return a.tl.x < b.tl.x;
}

} // namespace

Region::Region()
{
}

Region::Region(int x1, int y1, int x2, int y2)
{
  reset(Rect(x1, y1, x2, y2));
}

Region::Region(const Rect& r)
{
  reset(r);
}

Region::Region(const Region& r) : rects(r.rects)
{
}

Region& Region::operator=(const Region& src)
{
  if (this != &src)
    rects = src.rects;
  return *this;
}

Region::~Region()
{
}

void Region::clear()
{
  rects.clear();
}

void Region::reset(const Rect& r)
{
  clear();
  append_if_not_empty(rects, r);
}

void Region::translate(const Point& delta)
{
  for (std::vector<Rect>::iterator i = rects.begin(); i != rects.end(); ++i)
    *i = i->translate(delta);
}

void Region::setOrderedRects(const std::vector<Rect>& ordered_rects)
{
  rects.clear();
  for (std::vector<Rect>::const_iterator i = ordered_rects.begin(); i != ordered_rects.end(); ++i)
    append_if_not_empty(rects, *i);
}

void Region::assign_intersect(const Region& r)
{
  std::vector<Rect> result;
  for (std::vector<Rect>::const_iterator a = rects.begin(); a != rects.end(); ++a) {
    for (std::vector<Rect>::const_iterator b = r.rects.begin(); b != r.rects.end(); ++b)
      append_if_not_empty(result, a->intersect(*b));
  }
  rects.swap(result);
}

void Region::assign_union(const Region& r)
{
  for (std::vector<Rect>::const_iterator i = r.rects.begin(); i != r.rects.end(); ++i)
    append_if_not_empty(rects, *i);
}

void Region::assign_subtract(const Region& r)
{
  std::vector<Rect> current(rects);
  for (std::vector<Rect>::const_iterator cut = r.rects.begin(); cut != r.rects.end(); ++cut) {
    std::vector<Rect> next;
    for (std::vector<Rect>::const_iterator source = current.begin(); source != current.end(); ++source) {
      const std::vector<Rect> pieces = subtract_rect(*source, *cut);
      next.insert(next.end(), pieces.begin(), pieces.end());
    }
    current.swap(next);
  }
  rects.swap(current);
}

Region Region::intersect(const Region& r) const
{
  Region result(*this);
  result.assign_intersect(r);
  return result;
}

Region Region::union_(const Region& r) const
{
  Region result(*this);
  result.assign_union(r);
  return result;
}

Region Region::subtract(const Region& r) const
{
  Region result(*this);
  result.assign_subtract(r);
  return result;
}

bool Region::equals(const Region& b) const
{
  std::vector<Rect> a_rects;
  std::vector<Rect> b_rects;
  get_rects(a_rects);
  b.get_rects(b_rects);
  if (a_rects.size() != b_rects.size())
    return false;
  for (std::size_t i = 0; i < a_rects.size(); ++i) {
    if (!a_rects[i].equals(b_rects[i]))
      return false;
  }
  return true;
}

bool Region::is_empty() const
{
  return rects.empty();
}

bool Region::get_rects(std::vector<Rect>& out_rects, bool left2right, bool topdown) const
{
  out_rects = rects;
  std::sort(out_rects.begin(), out_rects.end(), rect_less_topdown_lefttoright);
  if (!topdown)
    std::reverse(out_rects.begin(), out_rects.end());
  if (!left2right) {
    std::stable_sort(out_rects.begin(), out_rects.end(), [](const Rect& a, const Rect& b) {
      if (a.tl.y != b.tl.y)
        return a.tl.y < b.tl.y;
      return a.tl.x > b.tl.x;
    });
    if (!topdown)
      std::reverse(out_rects.begin(), out_rects.end());
  }
  return !out_rects.empty();
}

Rect Region::get_bounding_rect() const
{
  if (rects.empty())
    return Rect(0, 0, 0, 0);

  Rect bounds = rects.front();
  for (std::vector<Rect>::const_iterator i = rects.begin() + 1; i != rects.end(); ++i)
    bounds = bounds.union_boundary(*i);
  return bounds;
}

int Region::Numrects()
{
  return static_cast<int>(rects.size());
}

void Region::debug_print(const char *prefix) const
{
  std::fprintf(stderr, "%s: %d rects\n", prefix ? prefix : "Region", static_cast<int>(rects.size()));
}
