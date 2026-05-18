// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerConfig.h"
#include "vncQtViewerConnectionPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include <cassert>

using uvnc::vncviewer::portable::ViewerConfig;
using uvnc::vncviewer::qtviewer::QtViewerConnectionPanel;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ViewerConfig config;
    config.SetHost("192.0.2.10");
    config.SetPort(5902);
    config.SetPassword("secret");
    config.SetShared(false);
    config.SetViewOnly(true);
    config.SetContinuousUpdates(true);
    config.SetUpdateIntervalMs(750);

    QtViewerConnectionPanel panel(config);
    assert(panel.Surface() != nullptr);
    assert(panel.StatusText() == "Disconnected");
    QLabel *serverClipboard = panel.findChild<QLabel *>("serverClipboardLabel");
    assert(serverClipboard && serverClipboard->text() == "Server clipboard: <none>");

    QLineEdit *host = panel.findChild<QLineEdit *>("hostEdit");
    QSpinBox *port = panel.findChild<QSpinBox *>("portSpin");
    QLineEdit *password = panel.findChild<QLineEdit *>("passwordEdit");
    QCheckBox *shared = panel.findChild<QCheckBox *>("sharedCheck");
    QCheckBox *viewOnly = panel.findChild<QCheckBox *>("viewOnlyCheck");
    QCheckBox *continuous = panel.findChild<QCheckBox *>("continuousCheck");
    QCheckBox *rememberPassword = panel.findChild<QCheckBox *>("rememberPasswordCheck");
    QCheckBox *autoReconnect = panel.findChild<QCheckBox *>("autoReconnectCheck");
    QSpinBox *interval = panel.findChild<QSpinBox *>("intervalSpin");
    QLineEdit *clipboard = panel.findChild<QLineEdit *>("clipboardEdit");
    QPushButton *sendClipboard = panel.findChild<QPushButton *>("sendClipboardButton");
    QPushButton *loadProfile = panel.findChild<QPushButton *>("loadProfileButton");
    QPushButton *saveProfile = panel.findChild<QPushButton *>("saveProfileButton");

    assert(host && host->text() == "192.0.2.10");
    assert(port && port->value() == 5902);
    assert(password && password->text() == "secret");
    assert(shared && !shared->isChecked());
    assert(viewOnly && viewOnly->isChecked());
    assert(continuous && continuous->isChecked());
    assert(rememberPassword && !rememberPassword->isChecked());
    assert(autoReconnect && !autoReconnect->isChecked());
    assert(interval && interval->value() == 750);
    assert(clipboard && clipboard->text().isEmpty());
    assert(sendClipboard && sendClipboard->text() == "Send clipboard");
    assert(loadProfile && loadProfile->text() == "Load profile");
    assert(saveProfile && saveProfile->text() == "Save profile");

    ViewerConfig current = panel.CurrentConfig();
    assert(current.Host() == "192.0.2.10");
    assert(current.Port() == 5902);
    assert(current.Password() == "secret");
    assert(!current.Shared());
    assert(current.ViewOnly());
    assert(current.ContinuousUpdates());
    assert(current.UpdateIntervalMs() == 750);
    return 0;
}
