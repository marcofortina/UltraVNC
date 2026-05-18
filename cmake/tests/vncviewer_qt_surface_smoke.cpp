// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtViewerSurface.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>

#include <cassert>
#include <vector>

using uvnc::vncviewer::qtviewer::QtViewerSurface;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QtViewerSurface surface;
    assert(surface.focusPolicy() == Qt::StrongFocus);
    assert(!surface.HasFramebuffer());

    std::vector<unsigned int> pixels(4 * 3, 0xff336699u);
    assert(surface.SetArgbFramebuffer(4, 3, pixels));
    assert(surface.HasFramebuffer());
    assert(surface.FramebufferSize() == QSize(4, 3));
    assert(!surface.SetArgbFramebuffer(4, 3, std::vector<unsigned int>()));

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::sendEvent(&surface, &keyPress);
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier);
    QApplication::sendEvent(&surface, &keyRelease);

    QMouseEvent mousePress(QEvent::MouseButtonPress, QPointF(10, 11), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&surface, &mousePress);
    QMouseEvent mouseMove(QEvent::MouseMove, QPointF(12, 13), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&surface, &mouseMove);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, QPointF(14, 15), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&surface, &mouseRelease);

    assert(surface.InputState().keyPresses == 1);
    assert(surface.InputState().keyReleases == 1);
    assert(surface.InputState().lastKey == Qt::Key_Control);
    assert(surface.InputState().mousePresses == 1);
    assert(surface.InputState().mouseMoves == 1);
    assert(surface.InputState().mouseReleases == 1);
    assert(surface.InputState().lastPointer == QPoint(14, 15));
    return 0;
}
