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

#include <cstdio>

namespace {

bool expect_rect(const rfb::Rect& rect, int x1, int y1, int x2, int y2)
{
  return rect.tl.x == x1 && rect.tl.y == y1 && rect.br.x == x2 && rect.br.y == y2;
}

int fail(const char* message)
{
  std::fprintf(stderr, "%s\n", message);
  return 1;
}

} // namespace

int main()
{
  rfb::Region2D screen(0, 0, 100, 100);
  rfb::SimpleUpdateTracker tracker(true);

  tracker.add_changed(rfb::Region2D(10, 10, 20, 20));
  tracker.add_cached(rfb::Region2D(30, 30, 40, 40));

  rfb::UpdateInfo info;
  tracker.flush_update(info, screen);

  if (info.changed.size() != 1 || !expect_rect(info.changed[0], 10, 10, 20, 20))
    return fail("changed region did not flush as expected");

  if (info.cached.size() != 1 || !expect_rect(info.cached[0], 30, 30, 40, 40))
    return fail("cached region did not flush as expected");

  if (!info.copied.empty())
    return fail("unexpected copied region in first flush");

  if (!tracker.is_empty())
    return fail("tracker should be empty after flushing visible regions");

  tracker.add_copied(rfb::Region2D(50, 50, 60, 60), rfb::Point(-5, -5));
  tracker.flush_update(info, screen);

  if (info.copied.size() != 1 || !expect_rect(info.copied[0], 50, 50, 60, 60))
    return fail("copied region did not flush as expected");

  if (info.copy_delta.x != -5 || info.copy_delta.y != -5)
    return fail("copy delta did not flush as expected");

  return 0;
}
