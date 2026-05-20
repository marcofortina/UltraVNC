// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#ifndef UVNC_WINVNC_QT_SERVER_SETTINGS_PANEL_H
#define UVNC_WINVNC_QT_SERVER_SETTINGS_PANEL_H

#include "vncPortableServerConfig.h"

#include <QWidget>

#include <QProcess>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QProcess;

namespace uvnc {
namespace winvnc {
namespace qtserver {

class QtServerSettingsPanel : public QWidget {
public:
    explicit QtServerSettingsPanel(QWidget *parent = nullptr);

    portable::ServerConfig CurrentConfig() const;
    QString GeneratedConfigText() const;
    QString GeneratedCommandLine() const;
    QString StatusText() const;
    QString VisualParityReport() const;
    bool LoadConfigText(const QString& text, QString *error = nullptr);
    bool ServerRunning() const;

private:
    void ValidateConfig(bool showDialog);
    void LoadConfig();
    void SaveConfig();
    void RefreshPreview();
    void StartServer(bool showDialog);
    void StopServer(bool showDialog);
    void RefreshRuntimeStatus();
    void RefreshRuntimeLog();
    void StartSystemdService(bool showDialog);
    void StopSystemdService(bool showDialog);
    void RefreshSystemdServiceStatus();
    QStringList SystemdArguments(const QString& action) const;
    QString WriteRuntimeConfig(QString *error) const;
    void SetStatus(const QString& status);
    void ShowError(const QString& message, bool showDialog);
    portable::ServerAuthMode SelectedAuthMode() const;
    portable::TransportSecurityMode SelectedTransportSecurity() const;
    portable::FileTransferMode SelectedFileTransferMode() const;

    QLineEdit *bindAddressEdit_;
    QSpinBox *portSpin_;
    QSpinBox *widthSpin_;
    QSpinBox *heightSpin_;
    QLineEdit *desktopNameEdit_;
    QComboBox *authModeCombo_;
    QLineEdit *passwordEdit_;
    QLineEdit *passwordFileEdit_;
    QLineEdit *authHelperEdit_;
    QLineEdit *dsmProviderEdit_;
    QCheckBox *allowNoAuthCheck_;
    QCheckBox *allowPublicNoAuthCheck_;
    QCheckBox *allowUnencryptedPublicCheck_;
    QComboBox *transportSecurityCombo_;
    QLineEdit *tlsCertEdit_;
    QLineEdit *tlsKeyEdit_;
    QComboBox *captureBackendCombo_;
    QComboBox *inputBackendCombo_;
    QComboBox *clipboardBackendCombo_;
    QLineEdit *logFileEdit_;
    QLineEdit *pidFileEdit_;
    QLineEdit *statusFileEdit_;
    QComboBox *fileTransferModeCombo_;
    QLineEdit *fileTransferRootEdit_;
    QCheckBox *fileTransferOverwriteCheck_;
    QSpinBox *maxSharedClientsSpin_;
    QSpinBox *updatePacingSpin_;
    QLineEdit *serverClipboardEdit_;
    QCheckBox *extendedClipboardCheck_;
    QSpinBox *clipboardLimitSpin_;
    QPushButton *validateButton_;
    QPushButton *loadButton_;
    QPushButton *saveButton_;
    QPushButton *refreshButton_;
    QLineEdit *serverExecutableEdit_;
    QLineEdit *runtimeConfigPathEdit_;
    QPushButton *startButton_;
    QPushButton *stopButton_;
    QPushButton *runtimeStatusButton_;
    QPushButton *runtimeLogButton_;
    QLabel *statusLabel_;
    QTextEdit *previewEdit_;
    QTextEdit *runtimeOutputEdit_;
    QLineEdit *systemdServiceEdit_;
    QComboBox *systemdScopeCombo_;
    QPushButton *systemdStartButton_;
    QPushButton *systemdStopButton_;
    QPushButton *systemdStatusButton_;
    QProcess *serverProcess_;
};

} // namespace qtserver
} // namespace winvnc
} // namespace uvnc

#endif // UVNC_WINVNC_QT_SERVER_SETTINGS_PANEL_H
