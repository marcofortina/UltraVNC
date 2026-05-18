// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtViewerSurface.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <cstring>

namespace uvnc {
namespace vncviewer {
namespace qtviewer {

QtViewerInputState::QtViewerInputState()
    : keyPresses(0),
      keyReleases(0),
      mousePresses(0),
      mouseReleases(0),
      mouseMoves(0),
      lastKey(0),
      lastPointer(0, 0),
      lastButtons(Qt::NoButton)
{
}

QtViewerSurface::QtViewerSurface(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(320, 200);
}

bool QtViewerSurface::SetArgbFramebuffer(int width, int height, const std::vector<unsigned int>& pixels)
{
    if (width <= 0 || height <= 0 || pixels.size() != static_cast<std::size_t>(width * height)) {
        return false;
    }

    framebuffer_ = QImage(width, height, QImage::Format_ARGB32);
    std::memcpy(framebuffer_.bits(), pixels.data(), pixels.size() * sizeof(unsigned int));
    update();
    return true;
}

void QtViewerSurface::ClearFramebuffer()
{
    framebuffer_ = QImage();
    update();
}

void QtViewerSurface::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
    if (framebuffer_.isNull()) {
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("No framebuffer"));
        return;
    }
    painter.drawImage(rect(), framebuffer_);
}

void QtViewerSurface::keyPressEvent(QKeyEvent *event)
{
    inputState_.keyPresses += 1;
    inputState_.lastKey = event->key();
    if (keyEventCallback_) {
        keyEventCallback_(event->key(), true);
    }
    event->accept();
}

void QtViewerSurface::keyReleaseEvent(QKeyEvent *event)
{
    inputState_.keyReleases += 1;
    inputState_.lastKey = event->key();
    if (keyEventCallback_) {
        keyEventCallback_(event->key(), false);
    }
    event->accept();
}

void QtViewerSurface::mousePressEvent(QMouseEvent *event)
{
    inputState_.mousePresses += 1;
    inputState_.lastPointer = event->position().toPoint();
    inputState_.lastButtons = event->buttons();
    if (pointerEventCallback_) {
        pointerEventCallback_(event->buttons(), inputState_.lastPointer);
    }
    event->accept();
}

void QtViewerSurface::mouseReleaseEvent(QMouseEvent *event)
{
    inputState_.mouseReleases += 1;
    inputState_.lastPointer = event->position().toPoint();
    inputState_.lastButtons = event->buttons();
    if (pointerEventCallback_) {
        pointerEventCallback_(event->buttons(), inputState_.lastPointer);
    }
    event->accept();
}

void QtViewerSurface::mouseMoveEvent(QMouseEvent *event)
{
    inputState_.mouseMoves += 1;
    inputState_.lastPointer = event->position().toPoint();
    inputState_.lastButtons = event->buttons();
    if (pointerEventCallback_) {
        pointerEventCallback_(event->buttons(), inputState_.lastPointer);
    }
    event->accept();
}

} // namespace qtviewer
} // namespace vncviewer
} // namespace uvnc
