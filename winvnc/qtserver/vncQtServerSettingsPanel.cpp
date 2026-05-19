// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtServerSettingsPanel.h"

#include "vncPortableFileTransfer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace uvnc {
namespace winvnc {
namespace qtserver {
namespace {

QString BoolText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

void AddComboItem(QComboBox *combo, const QString& label, int value)
{
    combo->addItem(label, value);
}

int ComboValue(const QComboBox *combo)
{
    return combo->currentData().toInt();
}

} // namespace

QtServerSettingsPanel::QtServerSettingsPanel(QWidget *parent)
    : QWidget(parent),
      bindAddressEdit_(new QLineEdit(QStringLiteral("127.0.0.1"))),
      portSpin_(new QSpinBox()),
      widthSpin_(new QSpinBox()),
      heightSpin_(new QSpinBox()),
      desktopNameEdit_(new QLineEdit(QStringLiteral("UltraVNC Linux Server"))),
      authModeCombo_(new QComboBox()),
      passwordEdit_(new QLineEdit()),
      passwordFileEdit_(new QLineEdit()),
      authHelperEdit_(new QLineEdit()),
      dsmProviderEdit_(new QLineEdit()),
      allowNoAuthCheck_(new QCheckBox(QStringLiteral("Allow no-auth lab mode"))),
      allowPublicNoAuthCheck_(new QCheckBox(QStringLiteral("Allow public no-auth"))),
      allowUnencryptedPublicCheck_(new QCheckBox(QStringLiteral("Allow unencrypted public bind"))),
      transportSecurityCombo_(new QComboBox()),
      tlsCertEdit_(new QLineEdit()),
      tlsKeyEdit_(new QLineEdit()),
      captureBackendCombo_(new QComboBox()),
      inputBackendCombo_(new QComboBox()),
      clipboardBackendCombo_(new QComboBox()),
      logFileEdit_(new QLineEdit()),
      pidFileEdit_(new QLineEdit()),
      statusFileEdit_(new QLineEdit()),
      fileTransferModeCombo_(new QComboBox()),
      fileTransferRootEdit_(new QLineEdit()),
      fileTransferOverwriteCheck_(new QCheckBox(QStringLiteral("Allow file-transfer overwrite"))),
      maxSharedClientsSpin_(new QSpinBox()),
      updatePacingSpin_(new QSpinBox()),
      serverClipboardEdit_(new QLineEdit()),
      extendedClipboardCheck_(new QCheckBox(QStringLiteral("Enable extended clipboard"))),
      clipboardLimitSpin_(new QSpinBox()),
      validateButton_(new QPushButton(QStringLiteral("Validate"))),
      saveButton_(new QPushButton(QStringLiteral("Save config"))),
      refreshButton_(new QPushButton(QStringLiteral("Refresh preview"))),
      serverExecutableEdit_(new QLineEdit(QStringLiteral("uvnc_winvnc_memory_server"))),
      runtimeConfigPathEdit_(new QLineEdit()),
      startButton_(new QPushButton(QStringLiteral("Start user server"))),
      stopButton_(new QPushButton(QStringLiteral("Stop"))),
      runtimeStatusButton_(new QPushButton(QStringLiteral("Refresh status"))),
      runtimeLogButton_(new QPushButton(QStringLiteral("Refresh log"))),
      statusLabel_(new QLabel(QStringLiteral("Not validated"))),
      previewEdit_(new QTextEdit()),
      runtimeOutputEdit_(new QTextEdit()),
      systemdServiceEdit_(new QLineEdit(QStringLiteral("uvnc-winvnc-memory-server.service"))),
      systemdScopeCombo_(new QComboBox()),
      systemdStartButton_(new QPushButton(QStringLiteral("Start service"))),
      systemdStopButton_(new QPushButton(QStringLiteral("Stop service"))),
      systemdStatusButton_(new QPushButton(QStringLiteral("Service status"))),
      serverProcess_(new QProcess(this))
{
    bindAddressEdit_->setObjectName(QStringLiteral("bindAddressEdit"));
    portSpin_->setObjectName(QStringLiteral("portSpin"));
    widthSpin_->setObjectName(QStringLiteral("widthSpin"));
    heightSpin_->setObjectName(QStringLiteral("heightSpin"));
    desktopNameEdit_->setObjectName(QStringLiteral("desktopNameEdit"));
    authModeCombo_->setObjectName(QStringLiteral("authModeCombo"));
    passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
    passwordFileEdit_->setObjectName(QStringLiteral("passwordFileEdit"));
    authHelperEdit_->setObjectName(QStringLiteral("authHelperEdit"));
    dsmProviderEdit_->setObjectName(QStringLiteral("dsmProviderEdit"));
    allowNoAuthCheck_->setObjectName(QStringLiteral("allowNoAuthCheck"));
    allowPublicNoAuthCheck_->setObjectName(QStringLiteral("allowPublicNoAuthCheck"));
    allowUnencryptedPublicCheck_->setObjectName(QStringLiteral("allowUnencryptedPublicCheck"));
    transportSecurityCombo_->setObjectName(QStringLiteral("transportSecurityCombo"));
    tlsCertEdit_->setObjectName(QStringLiteral("tlsCertEdit"));
    tlsKeyEdit_->setObjectName(QStringLiteral("tlsKeyEdit"));
    captureBackendCombo_->setObjectName(QStringLiteral("captureBackendCombo"));
    inputBackendCombo_->setObjectName(QStringLiteral("inputBackendCombo"));
    clipboardBackendCombo_->setObjectName(QStringLiteral("clipboardBackendCombo"));
    logFileEdit_->setObjectName(QStringLiteral("logFileEdit"));
    pidFileEdit_->setObjectName(QStringLiteral("pidFileEdit"));
    statusFileEdit_->setObjectName(QStringLiteral("statusFileEdit"));
    fileTransferModeCombo_->setObjectName(QStringLiteral("fileTransferModeCombo"));
    fileTransferRootEdit_->setObjectName(QStringLiteral("fileTransferRootEdit"));
    fileTransferOverwriteCheck_->setObjectName(QStringLiteral("fileTransferOverwriteCheck"));
    maxSharedClientsSpin_->setObjectName(QStringLiteral("maxSharedClientsSpin"));
    updatePacingSpin_->setObjectName(QStringLiteral("updatePacingSpin"));
    serverClipboardEdit_->setObjectName(QStringLiteral("serverClipboardEdit"));
    extendedClipboardCheck_->setObjectName(QStringLiteral("extendedClipboardCheck"));
    clipboardLimitSpin_->setObjectName(QStringLiteral("clipboardLimitSpin"));
    validateButton_->setObjectName(QStringLiteral("validateButton"));
    saveButton_->setObjectName(QStringLiteral("saveButton"));
    refreshButton_->setObjectName(QStringLiteral("refreshButton"));
    serverExecutableEdit_->setObjectName(QStringLiteral("serverExecutableEdit"));
    runtimeConfigPathEdit_->setObjectName(QStringLiteral("runtimeConfigPathEdit"));
    startButton_->setObjectName(QStringLiteral("startButton"));
    stopButton_->setObjectName(QStringLiteral("stopButton"));
    runtimeStatusButton_->setObjectName(QStringLiteral("runtimeStatusButton"));
    runtimeLogButton_->setObjectName(QStringLiteral("runtimeLogButton"));
    systemdServiceEdit_->setObjectName(QStringLiteral("systemdServiceEdit"));
    systemdScopeCombo_->setObjectName(QStringLiteral("systemdScopeCombo"));
    systemdStartButton_->setObjectName(QStringLiteral("systemdStartButton"));
    systemdStopButton_->setObjectName(QStringLiteral("systemdStopButton"));
    systemdStatusButton_->setObjectName(QStringLiteral("systemdStatusButton"));
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    previewEdit_->setObjectName(QStringLiteral("previewEdit"));
    runtimeOutputEdit_->setObjectName(QStringLiteral("runtimeOutputEdit"));

    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5900);
    widthSpin_->setRange(1, 16384);
    heightSpin_->setRange(1, 16384);
    widthSpin_->setValue(1024);
    heightSpin_->setValue(768);
    maxSharedClientsSpin_->setRange(1, 64);
    maxSharedClientsSpin_->setValue(8);
    updatePacingSpin_->setRange(0, 5000);
    clipboardLimitSpin_->setRange(1, 100 * 1024 * 1024);
    clipboardLimitSpin_->setValue(1024 * 1024);

    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordFileEdit_->setPlaceholderText(QStringLiteral("/etc/ultravnc/vnc-password"));
    authHelperEdit_->setPlaceholderText(QStringLiteral("/usr/local/libexec/uvnc-mslogon-auth"));
    dsmProviderEdit_->setPlaceholderText(QStringLiteral("/usr/local/lib/ultravnc/dsm-provider.so"));
    logFileEdit_->setPlaceholderText(QStringLiteral("/var/log/ultravnc/winvnc.log"));
    pidFileEdit_->setPlaceholderText(QStringLiteral("/run/ultravnc/winvnc.pid"));
    statusFileEdit_->setPlaceholderText(QStringLiteral("/run/ultravnc/winvnc.status"));
    runtimeConfigPathEdit_->setPlaceholderText(QStringLiteral("empty = write a temporary runtime config"));
    systemdServiceEdit_->setPlaceholderText(QStringLiteral("uvnc-winvnc-memory-server.service"));
    systemdScopeCombo_->addItem(QStringLiteral("User service"), QStringLiteral("user"));
    systemdScopeCombo_->addItem(QStringLiteral("System service"), QStringLiteral("system"));
    previewEdit_->setReadOnly(true);
    previewEdit_->setMinimumHeight(220);
    runtimeOutputEdit_->setReadOnly(true);
    runtimeOutputEdit_->setMinimumHeight(120);
    extendedClipboardCheck_->setChecked(true);

    AddComboItem(authModeCombo_, QStringLiteral("No auth (lab only)"), static_cast<int>(portable::ServerAuthMode::NoAuth));
    AddComboItem(authModeCombo_, QStringLiteral("VNC password"), static_cast<int>(portable::ServerAuthMode::VncPassword));
    AddComboItem(authModeCombo_, QStringLiteral("MSLogonII helper"), static_cast<int>(portable::ServerAuthMode::MsLogonII));
    authModeCombo_->setCurrentIndex(1);

    AddComboItem(transportSecurityCombo_, QStringLiteral("None"), static_cast<int>(portable::TransportSecurityMode::None));
    AddComboItem(transportSecurityCombo_, QStringLiteral("VeNCrypt X509Vnc"), static_cast<int>(portable::TransportSecurityMode::VeNCryptX509Vnc));

    captureBackendCombo_->addItem(QStringLiteral("auto"));
    captureBackendCombo_->addItem(QStringLiteral("x11"));
    captureBackendCombo_->addItem(QStringLiteral("pipewire"));
    captureBackendCombo_->addItem(QStringLiteral("none"));
    inputBackendCombo_->addItem(QStringLiteral("none"));
    inputBackendCombo_->addItem(QStringLiteral("xtest"));
    clipboardBackendCombo_->addItem(QStringLiteral("memory"));
    clipboardBackendCombo_->addItem(QStringLiteral("x11"));

    AddComboItem(fileTransferModeCombo_, QStringLiteral("Disabled"), static_cast<int>(portable::FileTransferMode::Disabled));
    AddComboItem(fileTransferModeCombo_, QStringLiteral("Reject only"), static_cast<int>(portable::FileTransferMode::RejectOnly));
    AddComboItem(fileTransferModeCombo_, QStringLiteral("Read only"), static_cast<int>(portable::FileTransferMode::ReadOnly));
    AddComboItem(fileTransferModeCombo_, QStringLiteral("Read/write"), static_cast<int>(portable::FileTransferMode::ReadWrite));

    QFormLayout *form = new QFormLayout();
    form->addRow(QStringLiteral("Bind address"), bindAddressEdit_);
    form->addRow(QStringLiteral("Port"), portSpin_);
    form->addRow(QStringLiteral("Width"), widthSpin_);
    form->addRow(QStringLiteral("Height"), heightSpin_);
    form->addRow(QStringLiteral("Desktop name"), desktopNameEdit_);
    form->addRow(QStringLiteral("Auth mode"), authModeCombo_);
    form->addRow(QStringLiteral("Password"), passwordEdit_);
    form->addRow(QStringLiteral("Password file"), passwordFileEdit_);
    form->addRow(QStringLiteral("Auth helper"), authHelperEdit_);
    form->addRow(QStringLiteral("DSM provider"), dsmProviderEdit_);
    form->addRow(QStringLiteral("Transport security"), transportSecurityCombo_);
    form->addRow(QStringLiteral("TLS certificate"), tlsCertEdit_);
    form->addRow(QStringLiteral("TLS private key"), tlsKeyEdit_);
    form->addRow(QStringLiteral("Capture backend"), captureBackendCombo_);
    form->addRow(QStringLiteral("Input backend"), inputBackendCombo_);
    form->addRow(QStringLiteral("Clipboard backend"), clipboardBackendCombo_);
    form->addRow(QStringLiteral("Log file"), logFileEdit_);
    form->addRow(QStringLiteral("PID file"), pidFileEdit_);
    form->addRow(QStringLiteral("Status file"), statusFileEdit_);
    form->addRow(QStringLiteral("File transfer"), fileTransferModeCombo_);
    form->addRow(QStringLiteral("File-transfer root"), fileTransferRootEdit_);
    form->addRow(QStringLiteral("Max shared clients"), maxSharedClientsSpin_);
    form->addRow(QStringLiteral("Update pacing ms"), updatePacingSpin_);
    form->addRow(QStringLiteral("Initial clipboard"), serverClipboardEdit_);
    form->addRow(QStringLiteral("Clipboard limit"), clipboardLimitSpin_);

    QHBoxLayout *flags = new QHBoxLayout();
    flags->addWidget(allowNoAuthCheck_);
    flags->addWidget(allowPublicNoAuthCheck_);
    flags->addWidget(allowUnencryptedPublicCheck_);
    flags->addWidget(fileTransferOverwriteCheck_);
    flags->addWidget(extendedClipboardCheck_);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(validateButton_);
    buttons->addWidget(saveButton_);
    buttons->addWidget(refreshButton_);

    QHBoxLayout *runtimeButtons = new QHBoxLayout();
    runtimeButtons->addWidget(startButton_);
    runtimeButtons->addWidget(stopButton_);
    runtimeButtons->addWidget(runtimeStatusButton_);
    runtimeButtons->addWidget(runtimeLogButton_);

    QFormLayout *systemdForm = new QFormLayout();
    systemdForm->addRow(QStringLiteral("Service name"), systemdServiceEdit_);
    systemdForm->addRow(QStringLiteral("Service scope"), systemdScopeCombo_);

    QHBoxLayout *systemdButtons = new QHBoxLayout();
    systemdButtons->addWidget(systemdStartButton_);
    systemdButtons->addWidget(systemdStopButton_);
    systemdButtons->addWidget(systemdStatusButton_);

    QWidget *settingsPage = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    settingsLayout->addLayout(form);
    settingsLayout->addLayout(flags);
    settingsLayout->addLayout(buttons);

    QWidget *runtimePage = new QWidget();
    QVBoxLayout *runtimeLayout = new QVBoxLayout(runtimePage);
    runtimeLayout->addLayout(runtimeButtons);
    runtimeLayout->addSpacing(8);
    runtimeLayout->addLayout(systemdForm);
    runtimeLayout->addLayout(systemdButtons);
    runtimeLayout->addWidget(runtimeOutputEdit_);

    QWidget *previewPage = new QWidget();
    QVBoxLayout *previewLayout = new QVBoxLayout(previewPage);
    previewLayout->addWidget(previewEdit_);

    QTabWidget *tabs = new QTabWidget();
    tabs->setObjectName(QStringLiteral("serverSettingsTabs"));
    tabs->addTab(settingsPage, QStringLiteral("Settings"));
    tabs->addTab(runtimePage, QStringLiteral("Runtime"));
    tabs->addTab(previewPage, QStringLiteral("Config preview"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *title = new QLabel(QStringLiteral("UltraVNC Linux Server"));
    title->setObjectName(QStringLiteral("serverSettingsTitle"));
    title->setStyleSheet(QStringLiteral("font-weight: 600; font-size: 16px;"));
    QLabel *classicParity = new QLabel(QStringLiteral("WinVNC-compatible settings layout: connections, authentication, transport, capture/input, file-transfer and service runtime"));
    classicParity->setObjectName(QStringLiteral("classicParityLabel"));
    classicParity->setWordWrap(true);
    classicParity->setStyleSheet(QStringLiteral("color: #555;"));
    layout->addWidget(title);
    layout->addWidget(classicParity);
    layout->addWidget(statusLabel_);
    layout->addWidget(tabs);

    QObject::connect(validateButton_, &QPushButton::clicked, this, [this]() { ValidateConfig(true); });
    QObject::connect(saveButton_, &QPushButton::clicked, this, [this]() { SaveConfig(); });
    QObject::connect(refreshButton_, &QPushButton::clicked, this, [this]() { RefreshPreview(); });
    QObject::connect(startButton_, &QPushButton::clicked, this, [this]() { StartServer(true); });
    QObject::connect(stopButton_, &QPushButton::clicked, this, [this]() { StopServer(true); });
    QObject::connect(runtimeStatusButton_, &QPushButton::clicked, this, [this]() { RefreshRuntimeStatus(); });
    QObject::connect(runtimeLogButton_, &QPushButton::clicked, this, [this]() { RefreshRuntimeLog(); });
    QObject::connect(systemdStartButton_, &QPushButton::clicked, this, [this]() { StartSystemdService(true); });
    QObject::connect(systemdStopButton_, &QPushButton::clicked, this, [this]() { StopSystemdService(true); });
    QObject::connect(systemdStatusButton_, &QPushButton::clicked, this, [this]() { RefreshSystemdServiceStatus(); });
    QObject::connect(serverProcess_, &QProcess::readyReadStandardOutput, this, [this]() { runtimeOutputEdit_->append(QString::fromUtf8(serverProcess_->readAllStandardOutput())); });
    QObject::connect(serverProcess_, &QProcess::readyReadStandardError, this, [this]() { runtimeOutputEdit_->append(QString::fromUtf8(serverProcess_->readAllStandardError())); });
    QObject::connect(serverProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        SetStatus(QStringLiteral("Server exited: code=%1 status=%2").arg(code).arg(status == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crashed")));
    });
    QObject::connect(authModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { RefreshPreview(); });
    QObject::connect(transportSecurityCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { RefreshPreview(); });
    QObject::connect(fileTransferModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { RefreshPreview(); });

    RefreshPreview();
}

portable::ServerAuthMode QtServerSettingsPanel::SelectedAuthMode() const
{
    return static_cast<portable::ServerAuthMode>(ComboValue(authModeCombo_));
}

portable::TransportSecurityMode QtServerSettingsPanel::SelectedTransportSecurity() const
{
    return static_cast<portable::TransportSecurityMode>(ComboValue(transportSecurityCombo_));
}

portable::FileTransferMode QtServerSettingsPanel::SelectedFileTransferMode() const
{
    return static_cast<portable::FileTransferMode>(ComboValue(fileTransferModeCombo_));
}

portable::ServerConfig QtServerSettingsPanel::CurrentConfig() const
{
    portable::ServerConfig config;
    config.SetBindAddress(bindAddressEdit_->text().toStdString());
    config.SetPort(static_cast<unsigned short>(portSpin_->value()));
    config.SetSize(static_cast<unsigned int>(widthSpin_->value()), static_cast<unsigned int>(heightSpin_->value()));
    config.SetDesktopName(desktopNameEdit_->text().toStdString());
    config.SetAuthMode(SelectedAuthMode());
    config.SetVncPassword(passwordEdit_->text().toStdString());
    config.SetAuthHelperPath(authHelperEdit_->text().toStdString());
    config.SetDsmProviderPath(dsmProviderEdit_->text().toStdString());
    config.SetAllowNoAuth(allowNoAuthCheck_->isChecked());
    config.SetAllowPublicNoAuth(allowPublicNoAuthCheck_->isChecked());
    config.SetAllowUnencryptedPublic(allowUnencryptedPublicCheck_->isChecked());
    config.SetTransportSecurity(SelectedTransportSecurity());
    config.SetTlsCertificateFile(tlsCertEdit_->text().toStdString());
    config.SetTlsPrivateKeyFile(tlsKeyEdit_->text().toStdString());
    config.SetFileTransferMode(SelectedFileTransferMode());
    config.SetFileTransferRoot(fileTransferRootEdit_->text().toStdString());
    config.SetFileTransferAllowOverwrite(fileTransferOverwriteCheck_->isChecked());
    config.SetMaxSharedClients(static_cast<unsigned int>(maxSharedClientsSpin_->value()));
    config.SetUpdatePacingMs(static_cast<unsigned int>(updatePacingSpin_->value()));
    config.SetServerCutText(serverClipboardEdit_->text().toStdString());
    config.SetExtendedClipboardEnabled(extendedClipboardCheck_->isChecked());
    config.SetExtendedClipboardTextLimit(static_cast<unsigned int>(clipboardLimitSpin_->value()));
    return config;
}

QString QtServerSettingsPanel::GeneratedConfigText() const
{
    const portable::ServerConfig config = CurrentConfig();
    QString text;
    text += QStringLiteral("bind_address=%1\n").arg(bindAddressEdit_->text());
    text += QStringLiteral("port=%1\n").arg(portSpin_->value());
    text += QStringLiteral("width=%1\n").arg(widthSpin_->value());
    text += QStringLiteral("height=%1\n").arg(heightSpin_->value());
    text += QStringLiteral("desktop_name=%1\n").arg(desktopNameEdit_->text());
    text += QStringLiteral("auth=%1\n").arg(QString::fromLatin1(portable::ServerAuthModeName(config.AuthMode())));
    if (config.AuthMode() == portable::ServerAuthMode::VncPassword) {
        if (!passwordFileEdit_->text().isEmpty()) {
            text += QStringLiteral("password_file=%1\n").arg(passwordFileEdit_->text());
        } else {
            text += QStringLiteral("# Use password_file in production instead of inline secrets.\n");
        }
    }
    if (config.AuthMode() == portable::ServerAuthMode::MsLogonII) {
        text += QStringLiteral("auth_helper=%1\n").arg(authHelperEdit_->text());
    }
    if (!dsmProviderEdit_->text().isEmpty()) {
        text += QStringLiteral("dsm_provider=%1\n").arg(dsmProviderEdit_->text());
    }
    text += QStringLiteral("allow_no_auth=%1\n").arg(BoolText(allowNoAuthCheck_->isChecked()));
    text += QStringLiteral("allow_public_no_auth=%1\n").arg(BoolText(allowPublicNoAuthCheck_->isChecked()));
    text += QStringLiteral("allow_unencrypted_public=%1\n").arg(BoolText(allowUnencryptedPublicCheck_->isChecked()));
    text += QStringLiteral("capture_backend=%1\n").arg(captureBackendCombo_->currentText());
    text += QStringLiteral("input_backend=%1\n").arg(inputBackendCombo_->currentText());
    text += QStringLiteral("clipboard_backend=%1\n").arg(clipboardBackendCombo_->currentText());
    if (!logFileEdit_->text().isEmpty()) text += QStringLiteral("log_file=%1\n").arg(logFileEdit_->text());
    if (!pidFileEdit_->text().isEmpty()) text += QStringLiteral("pid_file=%1\n").arg(pidFileEdit_->text());
    if (!statusFileEdit_->text().isEmpty()) text += QStringLiteral("status_file=%1\n").arg(statusFileEdit_->text());
    text += QStringLiteral("transport_security=%1\n").arg(QString::fromLatin1(portable::TransportSecurityModeName(config.TransportSecurity())));
    if (config.TransportSecurity() == portable::TransportSecurityMode::VeNCryptX509Vnc) {
        text += QStringLiteral("tls_certificate_file=%1\n").arg(tlsCertEdit_->text());
        text += QStringLiteral("tls_private_key_file=%1\n").arg(tlsKeyEdit_->text());
    }
    text += QStringLiteral("file_transfer_mode=%1\n").arg(QString::fromLatin1(portable::FileTransferModeName(config.FileTransferModeValue())));
    if (!fileTransferRootEdit_->text().isEmpty()) {
        text += QStringLiteral("file_transfer_root=%1\n").arg(fileTransferRootEdit_->text());
        text += QStringLiteral("file_transfer_allow_overwrite=%1\n").arg(BoolText(fileTransferOverwriteCheck_->isChecked()));
    }
    text += QStringLiteral("max_shared_clients=%1\n").arg(maxSharedClientsSpin_->value());
    text += QStringLiteral("update_pacing_ms=%1\n").arg(updatePacingSpin_->value());
    text += QStringLiteral("extended_clipboard=%1\n").arg(BoolText(extendedClipboardCheck_->isChecked()));
    text += QStringLiteral("extended_clipboard_text_limit=%1\n").arg(clipboardLimitSpin_->value());
    if (!serverClipboardEdit_->text().isEmpty()) {
        text += QStringLiteral("server_cut_text=%1\n").arg(serverClipboardEdit_->text());
    }
    return text;
}

QString QtServerSettingsPanel::StatusText() const
{
    return statusLabel_->text();
}

bool QtServerSettingsPanel::ServerRunning() const
{
    return serverProcess_->state() != QProcess::NotRunning;
}


QString QtServerSettingsPanel::WriteRuntimeConfig(QString *error) const
{
    QString path = runtimeConfigPathEdit_->text();
    if (path.isEmpty()) {
        QTemporaryFile tmp(QStringLiteral("/tmp/uvnc-qt-server-settings-XXXXXX.conf"));
        tmp.setAutoRemove(false);
        if (!tmp.open()) {
            if (error) *error = QStringLiteral("Failed to create temporary config");
            return QString();
        }
        path = tmp.fileName();
        tmp.write(GeneratedConfigText().toUtf8());
        tmp.close();
        return path;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = QStringLiteral("Failed to open runtime config for writing");
        return QString();
    }
    file.write(GeneratedConfigText().toUtf8());
    if (!file.commit()) {
        if (error) *error = QStringLiteral("Failed to write runtime config");
        return QString();
    }
    return path;
}

void QtServerSettingsPanel::StartServer(bool showDialog)
{
    if (ServerRunning()) {
        ShowError(QStringLiteral("Server process is already running"), showDialog);
        return;
    }
    ValidateConfig(false);
    if (StatusText() != QStringLiteral("Configuration is valid")) {
        return;
    }
    QString error;
    const QString configPath = WriteRuntimeConfig(&error);
    if (configPath.isEmpty()) {
        ShowError(error, showDialog);
        return;
    }
    QString executable = serverExecutableEdit_->text().trimmed();
    if (executable.isEmpty()) {
        executable = QStringLiteral("uvnc_winvnc_memory_server");
    }
    const QString resolved = QStandardPaths::findExecutable(executable);
    if (!resolved.isEmpty()) {
        executable = resolved;
    }
    runtimeOutputEdit_->clear();
    serverProcess_->setProgram(executable);
    serverProcess_->setArguments(QStringList() << QStringLiteral("--config") << configPath << QStringLiteral("--serve-forever"));
    serverProcess_->start();
    if (!serverProcess_->waitForStarted(3000)) {
        ShowError(QStringLiteral("Failed to start server process"), showDialog);
        return;
    }
    SetStatus(QStringLiteral("Server process started"));
}

void QtServerSettingsPanel::StopServer(bool showDialog)
{
    if (!ServerRunning()) {
        SetStatus(QStringLiteral("Server process is not running"));
        return;
    }
    serverProcess_->terminate();
    if (!serverProcess_->waitForFinished(3000)) {
        serverProcess_->kill();
        serverProcess_->waitForFinished(3000);
    }
    SetStatus(QStringLiteral("Server process stopped"));
    if (showDialog) {
        QMessageBox::information(this, QStringLiteral("UltraVNC server settings"), QStringLiteral("Server process stopped."));
    }
}

void QtServerSettingsPanel::RefreshRuntimeStatus()
{
    QString text;
    text += QStringLiteral("process_running=%1\n").arg(ServerRunning() ? QStringLiteral("yes") : QStringLiteral("no"));
    if (!statusFileEdit_->text().isEmpty()) {
        QFile file(statusFileEdit_->text());
        if (file.open(QIODevice::ReadOnly)) {
            text += QStringLiteral("status_file:\n%1\n").arg(QString::fromUtf8(file.readAll()));
        } else {
            text += QStringLiteral("status_file=unreadable\n");
        }
    }
    runtimeOutputEdit_->setPlainText(text);
    SetStatus(QStringLiteral("Runtime status refreshed"));
}

void QtServerSettingsPanel::RefreshRuntimeLog()
{
    if (logFileEdit_->text().isEmpty()) {
        ShowError(QStringLiteral("Log file path is empty"), false);
        return;
    }
    QFile file(logFileEdit_->text());
    if (!file.open(QIODevice::ReadOnly)) {
        ShowError(QStringLiteral("Failed to open log file"), false);
        return;
    }
    const QByteArray bytes = file.readAll();
    runtimeOutputEdit_->setPlainText(QString::fromUtf8(bytes.right(64 * 1024)));
    SetStatus(QStringLiteral("Runtime log refreshed"));
}


QStringList QtServerSettingsPanel::SystemdArguments(const QString& action) const
{
    QStringList args;
    if (systemdScopeCombo_->currentData().toString() == QStringLiteral("user")) {
        args << QStringLiteral("--user");
    }
    args << action << systemdServiceEdit_->text().trimmed();
    return args;
}

void QtServerSettingsPanel::StartSystemdService(bool showDialog)
{
    if (systemdServiceEdit_->text().trimmed().isEmpty()) {
        ShowError(QStringLiteral("Systemd service name is empty"), showDialog);
        return;
    }
    QProcess systemctl;
    systemctl.start(QStringLiteral("systemctl"), SystemdArguments(QStringLiteral("start")));
    systemctl.waitForFinished(10000);
    runtimeOutputEdit_->setPlainText(QString::fromUtf8(systemctl.readAllStandardOutput()) + QString::fromUtf8(systemctl.readAllStandardError()));
    if (systemctl.exitStatus() != QProcess::NormalExit || systemctl.exitCode() != 0) {
        ShowError(QStringLiteral("Failed to start systemd service"), showDialog);
        return;
    }
    SetStatus(QStringLiteral("Systemd service start requested"));
}

void QtServerSettingsPanel::StopSystemdService(bool showDialog)
{
    if (systemdServiceEdit_->text().trimmed().isEmpty()) {
        ShowError(QStringLiteral("Systemd service name is empty"), showDialog);
        return;
    }
    QProcess systemctl;
    systemctl.start(QStringLiteral("systemctl"), SystemdArguments(QStringLiteral("stop")));
    systemctl.waitForFinished(10000);
    runtimeOutputEdit_->setPlainText(QString::fromUtf8(systemctl.readAllStandardOutput()) + QString::fromUtf8(systemctl.readAllStandardError()));
    if (systemctl.exitStatus() != QProcess::NormalExit || systemctl.exitCode() != 0) {
        ShowError(QStringLiteral("Failed to stop systemd service"), showDialog);
        return;
    }
    SetStatus(QStringLiteral("Systemd service stop requested"));
}

void QtServerSettingsPanel::RefreshSystemdServiceStatus()
{
    if (systemdServiceEdit_->text().trimmed().isEmpty()) {
        ShowError(QStringLiteral("Systemd service name is empty"), false);
        return;
    }
    QProcess systemctl;
    systemctl.start(QStringLiteral("systemctl"), SystemdArguments(QStringLiteral("status")) << QStringLiteral("--no-pager"));
    systemctl.waitForFinished(10000);
    runtimeOutputEdit_->setPlainText(QString::fromUtf8(systemctl.readAllStandardOutput()) + QString::fromUtf8(systemctl.readAllStandardError()));
    SetStatus(QStringLiteral("Systemd service status refreshed"));
}

void QtServerSettingsPanel::ValidateConfig(bool showDialog)
{
    const portable::ServerConfig config = CurrentConfig();
    std::string error;
    if (!config.Validate(&error)) {
        ShowError(QString::fromStdString(error), showDialog);
        return;
    }
    SetStatus(QStringLiteral("Configuration is valid"));
    if (showDialog) {
        QMessageBox::information(this, QStringLiteral("UltraVNC server settings"), QStringLiteral("Configuration is valid."));
    }
}

void QtServerSettingsPanel::SaveConfig()
{
    ValidateConfig(false);
    if (StatusText() != QStringLiteral("Configuration is valid")) {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save UltraVNC Linux server config"), QString(), QStringLiteral("Config files (*.conf);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ShowError(QStringLiteral("Failed to open config file for writing"), true);
        return;
    }
    const QByteArray bytes = GeneratedConfigText().toUtf8();
    file.write(bytes);
    if (!file.commit()) {
        ShowError(QStringLiteral("Failed to save config file"), true);
        return;
    }
    SetStatus(QStringLiteral("Configuration saved"));
}

void QtServerSettingsPanel::RefreshPreview()
{
    previewEdit_->setPlainText(GeneratedConfigText());
}

void QtServerSettingsPanel::SetStatus(const QString& status)
{
    statusLabel_->setText(status);
    RefreshPreview();
}

void QtServerSettingsPanel::ShowError(const QString& message, bool showDialog)
{
    SetStatus(QStringLiteral("Error: %1").arg(message));
    if (showDialog) {
        QMessageBox::warning(this, QStringLiteral("UltraVNC server settings"), message);
    }
}

} // namespace qtserver
} // namespace winvnc
} // namespace uvnc
