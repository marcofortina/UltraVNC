// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtServerSettingsPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

#include <cassert>

using uvnc::winvnc::portable::ServerAuthMode;
using uvnc::winvnc::portable::TransportSecurityMode;
using uvnc::winvnc::qtserver::QtServerSettingsPanel;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QtServerSettingsPanel panel;

    QLineEdit *bind = panel.findChild<QLineEdit *>("bindAddressEdit");
    QSpinBox *port = panel.findChild<QSpinBox *>("portSpin");
    QSpinBox *width = panel.findChild<QSpinBox *>("widthSpin");
    QSpinBox *height = panel.findChild<QSpinBox *>("heightSpin");
    QLineEdit *desktop = panel.findChild<QLineEdit *>("desktopNameEdit");
    QComboBox *auth = panel.findChild<QComboBox *>("authModeCombo");
    QLineEdit *password = panel.findChild<QLineEdit *>("passwordEdit");
    QLineEdit *authHelper = panel.findChild<QLineEdit *>("authHelperEdit");
    QComboBox *transport = panel.findChild<QComboBox *>("transportSecurityCombo");
    QTextEdit *preview = panel.findChild<QTextEdit *>("previewEdit");

    assert(bind && bind->text() == "127.0.0.1");
    assert(port && port->value() == 5900);
    assert(width && width->value() == 1024);
    assert(height && height->value() == 768);
    assert(desktop && !desktop->text().isEmpty());
    assert(auth && auth->currentData().toInt() == static_cast<int>(ServerAuthMode::VncPassword));
    assert(password && password->echoMode() == QLineEdit::Password);
    assert(authHelper);
    assert(transport && transport->currentData().toInt() == static_cast<int>(TransportSecurityMode::None));
    assert(preview && preview->toPlainText().contains("auth=vnc-password"));

    password->setText("secret");
    assert(panel.CurrentConfig().Validate());

    auth->setCurrentIndex(auth->findData(static_cast<int>(ServerAuthMode::MsLogonII)));
    authHelper->setText("/tmp/uvnc-auth-helper");
    assert(panel.CurrentConfig().Validate());
    assert(panel.GeneratedConfigText().contains("auth=mslogon-ii"));
    assert(panel.GeneratedConfigText().contains("auth_helper=/tmp/uvnc-auth-helper"));

    return 0;
}
