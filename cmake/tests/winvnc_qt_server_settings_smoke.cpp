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
    QLineEdit *passwordFile = panel.findChild<QLineEdit *>("passwordFileEdit");
    QLineEdit *authHelper = panel.findChild<QLineEdit *>("authHelperEdit");
    QComboBox *transport = panel.findChild<QComboBox *>("transportSecurityCombo");
    QComboBox *capture = panel.findChild<QComboBox *>("captureBackendCombo");
    QComboBox *input = panel.findChild<QComboBox *>("inputBackendCombo");
    QComboBox *clipboard = panel.findChild<QComboBox *>("clipboardBackendCombo");
    QLineEdit *logFile = panel.findChild<QLineEdit *>("logFileEdit");
    QLineEdit *pidFile = panel.findChild<QLineEdit *>("pidFileEdit");
    QLineEdit *statusFile = panel.findChild<QLineEdit *>("statusFileEdit");
    QTextEdit *preview = panel.findChild<QTextEdit *>("previewEdit");

    assert(bind && bind->text() == "127.0.0.1");
    assert(port && port->value() == 5900);
    assert(width && width->value() == 1024);
    assert(height && height->value() == 768);
    assert(desktop && !desktop->text().isEmpty());
    assert(auth && auth->currentData().toInt() == static_cast<int>(ServerAuthMode::VncPassword));
    assert(password && password->echoMode() == QLineEdit::Password);
    assert(passwordFile);
    assert(authHelper);
    assert(transport && transport->currentData().toInt() == static_cast<int>(TransportSecurityMode::None));
    assert(capture && capture->currentText() == "auto");
    assert(input && input->currentText() == "none");
    assert(clipboard && clipboard->currentText() == "memory");
    assert(logFile && pidFile && statusFile);
    assert(preview && preview->toPlainText().contains("auth=vnc-password"));

    password->setText("secret");
    passwordFile->setText("/etc/ultravnc/vnc-password");
    logFile->setText("/var/log/ultravnc/winvnc.log");
    pidFile->setText("/run/ultravnc/winvnc.pid");
    statusFile->setText("/run/ultravnc/winvnc.status");
    assert(panel.CurrentConfig().Validate());
    assert(panel.GeneratedConfigText().contains("password_file=/etc/ultravnc/vnc-password"));
    assert(panel.GeneratedConfigText().contains("capture_backend=auto"));
    assert(panel.GeneratedConfigText().contains("log_file=/var/log/ultravnc/winvnc.log"));

    auth->setCurrentIndex(auth->findData(static_cast<int>(ServerAuthMode::MsLogonII)));
    authHelper->setText("/tmp/uvnc-auth-helper");
    assert(panel.CurrentConfig().Validate());
    assert(panel.GeneratedConfigText().contains("auth=mslogon-ii"));
    assert(panel.GeneratedConfigText().contains("auth_helper=/tmp/uvnc-auth-helper"));

    return 0;
}
