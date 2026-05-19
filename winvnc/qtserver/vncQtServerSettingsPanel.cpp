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
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
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
      statusLabel_(new QLabel(QStringLiteral("Not validated"))),
      previewEdit_(new QTextEdit())
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
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    previewEdit_->setObjectName(QStringLiteral("previewEdit"));

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
    logFileEdit_->setPlaceholderText(QStringLiteral("/var/log/ultravnc/winvnc.log"));
    pidFileEdit_->setPlaceholderText(QStringLiteral("/run/ultravnc/winvnc.pid"));
    statusFileEdit_->setPlaceholderText(QStringLiteral("/run/ultravnc/winvnc.status"));
    previewEdit_->setReadOnly(true);
    previewEdit_->setMinimumHeight(220);
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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(flags);
    layout->addLayout(buttons);
    layout->addWidget(statusLabel_);
    layout->addWidget(previewEdit_);

    QObject::connect(validateButton_, &QPushButton::clicked, this, [this]() { ValidateConfig(true); });
    QObject::connect(saveButton_, &QPushButton::clicked, this, [this]() { SaveConfig(); });
    QObject::connect(refreshButton_, &QPushButton::clicked, this, [this]() { RefreshPreview(); });
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
