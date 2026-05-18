// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_QT_VIEWER_SURFACE_H
#define UVNC_VNCVIEWER_QT_VIEWER_SURFACE_H

#include <QImage>
#include <QPoint>
#include <QWidget>

#include <vector>

namespace uvnc {
namespace vncviewer {
namespace qtviewer {

struct QtViewerInputState {
    QtViewerInputState();

    int keyPresses;
    int keyReleases;
    int mousePresses;
    int mouseReleases;
    int mouseMoves;
    int lastKey;
    QPoint lastPointer;
    Qt::MouseButtons lastButtons;
};

class QtViewerSurface : public QWidget {
public:
    explicit QtViewerSurface(QWidget *parent = nullptr);

    bool HasFramebuffer() const { return !framebuffer_.isNull(); }
    QSize FramebufferSize() const { return framebuffer_.size(); }
    const QtViewerInputState& InputState() const { return inputState_; }

    bool SetArgbFramebuffer(int width, int height, const std::vector<unsigned int>& pixels);
    void ClearFramebuffer();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QImage framebuffer_;
    QtViewerInputState inputState_;
};

} // namespace qtviewer
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_QT_VIEWER_SURFACE_H
