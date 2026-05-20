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
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
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
    QLineEdit *dsmProvider = panel.findChild<QLineEdit *>("dsmProviderEdit");
    QComboBox *transport = panel.findChild<QComboBox *>("transportSecurityCombo");
    QComboBox *capture = panel.findChild<QComboBox *>("captureBackendCombo");
    QComboBox *input = panel.findChild<QComboBox *>("inputBackendCombo");
    QComboBox *clipboard = panel.findChild<QComboBox *>("clipboardBackendCombo");
    QLineEdit *logFile = panel.findChild<QLineEdit *>("logFileEdit");
    QLineEdit *pidFile = panel.findChild<QLineEdit *>("pidFileEdit");
    QLineEdit *statusFile = panel.findChild<QLineEdit *>("statusFileEdit");
    QLineEdit *serverExecutable = panel.findChild<QLineEdit *>("serverExecutableEdit");
    QLineEdit *runtimeConfigPath = panel.findChild<QLineEdit *>("runtimeConfigPathEdit");
    QPushButton *startButton = panel.findChild<QPushButton *>("startButton");
    QPushButton *stopButton = panel.findChild<QPushButton *>("stopButton");
    QPushButton *runtimeStatusButton = panel.findChild<QPushButton *>("runtimeStatusButton");
    QPushButton *runtimeLogButton = panel.findChild<QPushButton *>("runtimeLogButton");
    QLineEdit *systemdService = panel.findChild<QLineEdit *>("systemdServiceEdit");
    QComboBox *systemdScope = panel.findChild<QComboBox *>("systemdScopeCombo");
    QPushButton *systemdStart = panel.findChild<QPushButton *>("systemdStartButton");
    QPushButton *systemdStop = panel.findChild<QPushButton *>("systemdStopButton");
    QPushButton *systemdStatus = panel.findChild<QPushButton *>("systemdStatusButton");
    QPushButton *loadButton = panel.findChild<QPushButton *>("loadButton");
    QTextEdit *preview = panel.findChild<QTextEdit *>("previewEdit");
    QTextEdit *runtimeOutput = panel.findChild<QTextEdit *>("runtimeOutputEdit");
    QLabel *title = panel.findChild<QLabel *>("serverSettingsTitle");
    QLabel *classicParity = panel.findChild<QLabel *>("classicParityLabel");
    QTabWidget *tabs = panel.findChild<QTabWidget *>("serverSettingsTabs");

    assert(bind && bind->text() == "127.0.0.1");
    assert(port && port->value() == 5900);
    assert(width && width->value() == 1024);
    assert(height && height->value() == 768);
    assert(desktop && !desktop->text().isEmpty());
    assert(auth && auth->currentData().toInt() == static_cast<int>(ServerAuthMode::VncPassword));
    assert(password && password->echoMode() == QLineEdit::Password);
    assert(passwordFile);
    assert(authHelper);
    assert(dsmProvider && dsmProvider->text().isEmpty());
    assert(transport && transport->currentData().toInt() == static_cast<int>(TransportSecurityMode::None));
    assert(capture && capture->currentText() == "auto");
    assert(input && input->currentText() == "none");
    assert(clipboard && clipboard->currentText() == "memory");
    assert(logFile && pidFile && statusFile);
    assert(serverExecutable);
    assert(serverExecutable->text() == "uvnc_winvnc_memory_server" || serverExecutable->text().endsWith("/uvnc_winvnc_memory_server"));
    assert(runtimeConfigPath && runtimeConfigPath->text().isEmpty());
    assert(startButton && startButton->text() == "Start user server");
    assert(stopButton && stopButton->text() == "Stop");
    assert(runtimeStatusButton && runtimeStatusButton->text() == "Refresh status");
    assert(runtimeLogButton && runtimeLogButton->text() == "Refresh log");
    assert(systemdService && systemdService->text() == "uvnc-winvnc-memory-server.service");
    assert(systemdScope && systemdScope->currentData().toString() == "user");
    assert(systemdStart && systemdStart->text() == "Start service");
    assert(systemdStop && systemdStop->text() == "Stop service");
    assert(systemdStatus && systemdStatus->text() == "Service status");
    assert(loadButton && loadButton->text() == "Load config");
    assert(preview && preview->toPlainText().contains("auth=vnc-password"));
    assert(runtimeOutput && runtimeOutput->toPlainText().isEmpty());
    assert(title && title->text() == "UltraVNC Linux Server");
    assert(classicParity && classicParity->text().contains("WinVNC-compatible settings layout"));
    assert(tabs && tabs->count() == 3);
    assert(tabs->tabText(0) == "Settings");
    assert(tabs->tabText(1) == "Runtime");
    assert(tabs->tabText(2) == "Config preview");
    assert(!panel.ServerRunning());

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

    dsmProvider->setText("/tmp/libuvnc-test-dsm.so");
    assert(panel.CurrentConfig().DsmProviderPath() == "/tmp/libuvnc-test-dsm.so");
    assert(panel.GeneratedConfigText().contains("dsm_provider=/tmp/libuvnc-test-dsm.so"));

    QString loadError;
    assert(panel.LoadConfigText(QStringLiteral(
        "bind_address=0.0.0.0\n"
        "port=5999\n"
        "width=1024\n"
        "height=768\n"
        "desktop_name=Loaded UltraVNC Server\n"
        "auth=mslogon-ii\n"
        "auth_helper=/usr/local/libexec/uvnc-auth\n"
        "dsm_provider=/usr/lib/ultravnc/SecureVNCPlugin.dsm\n"
        "transport_security=vencrypt-x509-vnc\n"
        "tls_certificate_file=/etc/ultravnc/server.crt\n"
        "tls_private_key_file=/etc/ultravnc/server.key\n"
        "capture_backend=x11\n"
        "input_backend=xtest\n"
        "clipboard_backend=x11\n"
        "log_file=/tmp/loaded.log\n"
        "pid_file=/tmp/loaded.pid\n"
        "status_file=/tmp/loaded.status\n"
        "file_transfer_mode=read-write\n"
        "file_transfer_root=/tmp\n"
        "file_transfer_allow_overwrite=true\n"
        "max_shared_clients=4\n"
        "update_pacing_ms=75\n"
        "extended_clipboard=false\n"
        "extended_clipboard_text_limit=4096\n"), &loadError));
    assert(loadError.isEmpty());
    assert(bindAddress->text() == "0.0.0.0");
    assert(port->value() == 5999);
    assert(width->value() == 1024);
    assert(height->value() == 768);
    assert(desktop->text() == "Loaded UltraVNC Server");
    assert(auth->currentData().toInt() == static_cast<int>(ServerAuthMode::MsLogonII));
    assert(authHelper->text() == "/usr/local/libexec/uvnc-auth");
    assert(dsmProvider->text() == "/usr/lib/ultravnc/SecureVNCPlugin.dsm");
    assert(panel.GeneratedConfigText().contains("auth=mslogon-ii"));
    assert(panel.StatusText() == "Configuration loaded");

    return 0;
}
