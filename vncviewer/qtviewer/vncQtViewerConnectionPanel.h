// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_VNCVIEWER_QT_VIEWER_CONNECTION_PANEL_H
#define UVNC_VNCVIEWER_QT_VIEWER_CONNECTION_PANEL_H

#include "vncPortableViewerConfig.h"
#include "vncQtViewerSurface.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;

namespace uvnc {
namespace vncviewer {
namespace qtviewer {

class QtViewerConnectionPanel : public QWidget {
public:
    explicit QtViewerConnectionPanel(const portable::ViewerConfig& initialConfig, QWidget *parent = nullptr);

    QtViewerSurface *Surface() const { return surface_; }
    QString StatusText() const;
    portable::ViewerConfig CurrentConfig() const;

private:
    void RequestUpdate(bool showDialogOnError);
    void StartContinuousUpdatesIfRequested();
    void StopContinuousUpdates();
    void SetStatus(const QString& status);
    void ShowError(const QString& message, bool showDialog);

    QLineEdit *hostEdit_;
    QSpinBox *portSpin_;
    QLineEdit *passwordEdit_;
    QCheckBox *sharedCheck_;
    QCheckBox *viewOnlyCheck_;
    QCheckBox *continuousCheck_;
    QSpinBox *intervalSpin_;
    QPushButton *connectButton_;
    QPushButton *updateButton_;
    QPushButton *reconnectButton_;
    QPushButton *disconnectButton_;
    QLabel *statusLabel_;
    QtViewerSurface *surface_;
    QTimer *continuousTimer_;
};

} // namespace qtviewer
} // namespace vncviewer
} // namespace uvnc

#endif // UVNC_VNCVIEWER_QT_VIEWER_CONNECTION_PANEL_H
