// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtViewerConnectionPanel.h"

#include "vncPortableViewerSession.h"

#include <QCheckBox>
#include <QClipboard>
#include <QFile>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <string>
#include <vector>

namespace uvnc {
namespace vncviewer {
namespace qtviewer {
namespace {

unsigned int ScaleColor(unsigned int value, unsigned int max)
{
    return max == 0 ? 0 : (value * 255u) / max;
}

bool RawUpdateToArgbPixels(const portable::ViewerSessionResult& result, std::vector<unsigned int>& pixels, std::string& error)
{
    const unsigned int bytesPerPixel = result.format.bitsPerPixel / 8;
    if (!result.update.received) {
        error = "no RFB framebuffer update was received";
        return false;
    }
    if (bytesPerPixel == 0 || bytesPerPixel > 4) {
        error = "unsupported RFB framebuffer update pixel size";
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(result.update.width) *
                                 static_cast<std::size_t>(result.update.height) *
                                 static_cast<std::size_t>(bytesPerPixel);
    if (result.update.pixels.size() != expected) {
        error = "unexpected RFB framebuffer update byte count";
        return false;
    }

    pixels.clear();
    pixels.reserve(static_cast<std::size_t>(result.update.width) * result.update.height);
    for (std::size_t offset = 0; offset < result.update.pixels.size(); offset += bytesPerPixel) {
        unsigned int raw = 0;
        if (result.format.bigEndian) {
            for (unsigned int i = 0; i < bytesPerPixel; ++i) {
                raw = (raw << 8) | result.update.pixels[offset + i];
            }
        } else {
            for (unsigned int i = 0; i < bytesPerPixel; ++i) {
                raw |= static_cast<unsigned int>(result.update.pixels[offset + i]) << (8 * i);
            }
        }
        const unsigned int red = ScaleColor((raw >> result.format.redShift) & result.format.redMax, result.format.redMax);
        const unsigned int green = ScaleColor((raw >> result.format.greenShift) & result.format.greenMax, result.format.greenMax);
        const unsigned int blue = ScaleColor((raw >> result.format.blueShift) & result.format.blueMax, result.format.blueMax);
        pixels.push_back(0xff000000u | (red << 16) | (green << 8) | blue);
    }
    return true;
}

CARD8 QtButtonsToRfbMask(Qt::MouseButtons buttons)
{
    CARD8 mask = 0;
    if (buttons & Qt::LeftButton) {
        mask |= 1;
    }
    if (buttons & Qt::MiddleButton) {
        mask |= 2;
    }
    if (buttons & Qt::RightButton) {
        mask |= 4;
    }
    return mask;
}

CARD32 QtKeyToRfbKeysym(int key)
{
    if (key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde) {
        return static_cast<CARD32>(key);
    }
    switch (key) {
    case Qt::Key_Backspace:
        return 0xff08;
    case Qt::Key_Tab:
        return 0xff09;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return 0xff0d;
    case Qt::Key_Escape:
        return 0xff1b;
    case Qt::Key_Insert:
        return 0xff63;
    case Qt::Key_Delete:
        return 0xffff;
    case Qt::Key_Home:
        return 0xff50;
    case Qt::Key_Left:
        return 0xff51;
    case Qt::Key_Up:
        return 0xff52;
    case Qt::Key_Right:
        return 0xff53;
    case Qt::Key_Down:
        return 0xff54;
    case Qt::Key_PageUp:
        return 0xff55;
    case Qt::Key_PageDown:
        return 0xff56;
    case Qt::Key_End:
        return 0xff57;
    case Qt::Key_Shift:
        return 0xffe1;
    case Qt::Key_Control:
        return 0xffe3;
    case Qt::Key_Alt:
        return 0xffe9;
    default:
        return 0;
    }
}

} // namespace

QtViewerConnectionPanel::QtViewerConnectionPanel(const portable::ViewerConfig& initialConfig, QWidget *parent)
    : QWidget(parent),
      hostEdit_(new QLineEdit(QString::fromStdString(initialConfig.Host()))),
      portSpin_(new QSpinBox()),
      passwordEdit_(new QLineEdit(QString::fromStdString(initialConfig.Password()))),
      sharedCheck_(new QCheckBox(QStringLiteral("Shared session"))),
      viewOnlyCheck_(new QCheckBox(QStringLiteral("View only"))),
      continuousCheck_(new QCheckBox(QStringLiteral("Continuous updates"))),
      autoReconnectCheck_(new QCheckBox(QStringLiteral("Auto reconnect"))),
      rememberPasswordCheck_(new QCheckBox(QStringLiteral("Remember password"))),
      tlsCheck_(new QCheckBox(QStringLiteral("VeNCrypt TLS"))),
      tlsInsecureCheck_(new QCheckBox(QStringLiteral("TLS insecure lab mode"))),
      tlsCaFileEdit_(new QLineEdit(QString::fromStdString(initialConfig.TlsCaFile()))),
      tlsServerNameEdit_(new QLineEdit(QString::fromStdString(initialConfig.TlsServerName()))),
      intervalSpin_(new QSpinBox()),
      connectButton_(new QPushButton(QStringLiteral("Connect"))),
      updateButton_(new QPushButton(QStringLiteral("Update once"))),
      reconnectButton_(new QPushButton(QStringLiteral("Reconnect"))),
      disconnectButton_(new QPushButton(QStringLiteral("Disconnect"))),
      loadProfileButton_(new QPushButton(QStringLiteral("Load profile"))),
      saveProfileButton_(new QPushButton(QStringLiteral("Save profile"))),
      clipboardEdit_(new QLineEdit()),
      sendClipboardButton_(new QPushButton(QStringLiteral("Send clipboard"))),
      remotePathEdit_(new QLineEdit(QStringLiteral("/"))),
      downloadOutputEdit_(new QLineEdit()),
      uploadInputEdit_(new QLineEdit()),
      remoteListButton_(new QPushButton(QStringLiteral("List remote"))),
      remoteDrivesButton_(new QPushButton(QStringLiteral("List roots"))),
      remoteDownloadButton_(new QPushButton(QStringLiteral("Download"))),
      remoteUploadButton_(new QPushButton(QStringLiteral("Upload"))),
      statusLabel_(new QLabel(QStringLiteral("Disconnected"))),
      serverClipboardLabel_(new QLabel(QStringLiteral("Server clipboard: <none>"))),
      remoteListingLabel_(new QLabel(QStringLiteral("Remote files: <none>"))),
      surface_(new QtViewerSurface()),
      continuousTimer_(new QTimer(this)),
      reconnectTimer_(new QTimer(this))
{
    hostEdit_->setObjectName(QStringLiteral("hostEdit"));
    portSpin_->setObjectName(QStringLiteral("portSpin"));
    passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
    sharedCheck_->setObjectName(QStringLiteral("sharedCheck"));
    viewOnlyCheck_->setObjectName(QStringLiteral("viewOnlyCheck"));
    continuousCheck_->setObjectName(QStringLiteral("continuousCheck"));
    autoReconnectCheck_->setObjectName(QStringLiteral("autoReconnectCheck"));
    rememberPasswordCheck_->setObjectName(QStringLiteral("rememberPasswordCheck"));
    tlsCheck_->setObjectName(QStringLiteral("tlsCheck"));
    tlsInsecureCheck_->setObjectName(QStringLiteral("tlsInsecureCheck"));
    tlsCaFileEdit_->setObjectName(QStringLiteral("tlsCaFileEdit"));
    tlsServerNameEdit_->setObjectName(QStringLiteral("tlsServerNameEdit"));
    intervalSpin_->setObjectName(QStringLiteral("intervalSpin"));
    clipboardEdit_->setObjectName(QStringLiteral("clipboardEdit"));
    sendClipboardButton_->setObjectName(QStringLiteral("sendClipboardButton"));
    remotePathEdit_->setObjectName(QStringLiteral("remotePathEdit"));
    downloadOutputEdit_->setObjectName(QStringLiteral("downloadOutputEdit"));
    uploadInputEdit_->setObjectName(QStringLiteral("uploadInputEdit"));
    remoteListButton_->setObjectName(QStringLiteral("remoteListButton"));
    remoteDrivesButton_->setObjectName(QStringLiteral("remoteDrivesButton"));
    remoteDownloadButton_->setObjectName(QStringLiteral("remoteDownloadButton"));
    remoteUploadButton_->setObjectName(QStringLiteral("remoteUploadButton"));
    loadProfileButton_->setObjectName(QStringLiteral("loadProfileButton"));
    saveProfileButton_->setObjectName(QStringLiteral("saveProfileButton"));
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    serverClipboardLabel_->setObjectName(QStringLiteral("serverClipboardLabel"));
    remoteListingLabel_->setObjectName(QStringLiteral("remoteListingLabel"));

    portSpin_->setRange(1, 65535);
    portSpin_->setValue(initialConfig.Port());
    passwordEdit_->setEchoMode(QLineEdit::Password);
    sharedCheck_->setChecked(initialConfig.Shared());
    viewOnlyCheck_->setChecked(initialConfig.ViewOnly());
    continuousCheck_->setChecked(initialConfig.ContinuousUpdates());
    intervalSpin_->setRange(100, 60000);
    intervalSpin_->setValue(static_cast<int>(initialConfig.UpdateIntervalMs()));
    tlsCheck_->setChecked(initialConfig.TransportSecurity() == portable::ViewerTransportSecurityMode::VeNCryptX509Vnc);
    tlsInsecureCheck_->setChecked(!initialConfig.TlsVerifyPeer());
    continuousTimer_->setSingleShot(false);
    reconnectTimer_->setSingleShot(true);

    QFormLayout *form = new QFormLayout();
    form->addRow(QStringLiteral("Host"), hostEdit_);
    form->addRow(QStringLiteral("Port"), portSpin_);
    form->addRow(QStringLiteral("Password"), passwordEdit_);
    form->addRow(QStringLiteral("TLS CA file"), tlsCaFileEdit_);
    form->addRow(QStringLiteral("TLS server name"), tlsServerNameEdit_);
    form->addRow(QStringLiteral("Update interval ms"), intervalSpin_);

    QHBoxLayout *options = new QHBoxLayout();
    options->addWidget(sharedCheck_);
    options->addWidget(viewOnlyCheck_);
    options->addWidget(continuousCheck_);
    options->addWidget(autoReconnectCheck_);
    options->addWidget(rememberPasswordCheck_);
    options->addWidget(tlsCheck_);
    options->addWidget(tlsInsecureCheck_);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(connectButton_);
    buttons->addWidget(updateButton_);
    buttons->addWidget(reconnectButton_);
    buttons->addWidget(disconnectButton_);

    QHBoxLayout *profiles = new QHBoxLayout();
    profiles->addWidget(loadProfileButton_);
    profiles->addWidget(saveProfileButton_);

    QHBoxLayout *clipboard = new QHBoxLayout();
    clipboard->addWidget(clipboardEdit_, 1);
    clipboard->addWidget(sendClipboardButton_);

    QHBoxLayout *fileTransfer = new QHBoxLayout();
    fileTransfer->addWidget(remotePathEdit_, 1);
    fileTransfer->addWidget(remoteListButton_);
    fileTransfer->addWidget(remoteDrivesButton_);
    fileTransfer->addWidget(downloadOutputEdit_, 1);
    fileTransfer->addWidget(remoteDownloadButton_);
    fileTransfer->addWidget(uploadInputEdit_, 1);
    fileTransfer->addWidget(remoteUploadButton_);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(options);
    layout->addLayout(buttons);
    layout->addLayout(profiles);
    layout->addLayout(clipboard);
    layout->addLayout(fileTransfer);
    layout->addWidget(statusLabel_);
    layout->addWidget(serverClipboardLabel_);
    layout->addWidget(remoteListingLabel_);
    layout->addWidget(surface_, 1);
    setLayout(layout);

    QObject::connect(connectButton_, &QPushButton::clicked, this, [this]() {
        RequestUpdate(true);
        StartContinuousUpdatesIfRequested();
    });
    QObject::connect(updateButton_, &QPushButton::clicked, this, [this]() {
        RequestUpdate(true);
    });
    QObject::connect(reconnectButton_, &QPushButton::clicked, this, [this]() {
        StopContinuousUpdates();
        session_.Disconnect();
        RequestUpdate(true);
        StartContinuousUpdatesIfRequested();
    });
    QObject::connect(disconnectButton_, &QPushButton::clicked, this, [this]() {
        StopContinuousUpdates();
        session_.Disconnect();
        SetStatus(QStringLiteral("Disconnected"));
    });
    QObject::connect(sendClipboardButton_, &QPushButton::clicked, this, [this]() {
        SendClipboardText(true);
    });
    QObject::connect(remoteListButton_, &QPushButton::clicked, this, [this]() {
        ListRemotePath(true);
    });
    QObject::connect(remoteDrivesButton_, &QPushButton::clicked, this, [this]() {
        ListRemoteDrives(true);
    });
    QObject::connect(remoteDownloadButton_, &QPushButton::clicked, this, [this]() {
        DownloadRemoteFile(true);
    });
    QObject::connect(remoteUploadButton_, &QPushButton::clicked, this, [this]() {
        UploadLocalFile(true);
    });
    QObject::connect(loadProfileButton_, &QPushButton::clicked, this, [this]() {
        LoadProfile();
    });
    QObject::connect(saveProfileButton_, &QPushButton::clicked, this, [this]() {
        SaveProfile();
    });
    QObject::connect(continuousTimer_, &QTimer::timeout, this, [this]() {
        RequestUpdate(false);
    });
    QObject::connect(reconnectTimer_, &QTimer::timeout, this, [this]() {
        RequestUpdate(false);
        StartContinuousUpdatesIfRequested();
    });
    surface_->SetKeyEventCallback([this](int key, bool down) {
        SendQtKeyEvent(key, down);
    });
    surface_->SetPointerEventCallback([this](Qt::MouseButtons buttons, const QPoint& position) {
        SendQtPointerEvent(buttons, position);
    });

    LoadProfile();
    SetStatus(QStringLiteral("Disconnected"));
}

QString QtViewerConnectionPanel::StatusText() const
{
    return statusLabel_->text();
}

portable::ViewerConfig QtViewerConnectionPanel::CurrentConfig() const
{
    portable::ViewerConfig config;
    config.SetHost(hostEdit_->text().toStdString());
    config.SetPort(static_cast<unsigned short>(portSpin_->value()));
    config.SetPassword(passwordEdit_->text().toStdString());
    config.SetShared(sharedCheck_->isChecked());
    config.SetViewOnly(viewOnlyCheck_->isChecked());
    config.SetContinuousUpdates(continuousCheck_->isChecked());
    config.SetUpdateIntervalMs(static_cast<unsigned int>(intervalSpin_->value()));
    config.SetTransportSecurity(tlsCheck_->isChecked() ? portable::ViewerTransportSecurityMode::VeNCryptX509Vnc : portable::ViewerTransportSecurityMode::None);
    config.SetTlsCaFile(tlsCaFileEdit_->text().toStdString());
    config.SetTlsServerName(tlsServerNameEdit_->text().toStdString());
    config.SetTlsVerifyPeer(!tlsInsecureCheck_->isChecked());
    return config;
}


void QtViewerConnectionPanel::LoadProfile()
{
    QSettings settings(QStringLiteral("UltraVNC"), QStringLiteral("QtViewer"));
    hostEdit_->setText(settings.value(QStringLiteral("host"), hostEdit_->text()).toString());
    portSpin_->setValue(settings.value(QStringLiteral("port"), portSpin_->value()).toInt());
    rememberPasswordCheck_->setChecked(settings.value(QStringLiteral("rememberPassword"), false).toBool());
    if (rememberPasswordCheck_->isChecked()) {
        passwordEdit_->setText(settings.value(QStringLiteral("password"), passwordEdit_->text()).toString());
    }
    sharedCheck_->setChecked(settings.value(QStringLiteral("shared"), sharedCheck_->isChecked()).toBool());
    viewOnlyCheck_->setChecked(settings.value(QStringLiteral("viewOnly"), viewOnlyCheck_->isChecked()).toBool());
    continuousCheck_->setChecked(settings.value(QStringLiteral("continuousUpdates"), continuousCheck_->isChecked()).toBool());
    autoReconnectCheck_->setChecked(settings.value(QStringLiteral("autoReconnect"), autoReconnectCheck_->isChecked()).toBool());
    tlsCheck_->setChecked(settings.value(QStringLiteral("tls"), tlsCheck_->isChecked()).toBool());
    tlsInsecureCheck_->setChecked(settings.value(QStringLiteral("tlsInsecure"), tlsInsecureCheck_->isChecked()).toBool());
    tlsCaFileEdit_->setText(settings.value(QStringLiteral("tlsCaFile"), tlsCaFileEdit_->text()).toString());
    tlsServerNameEdit_->setText(settings.value(QStringLiteral("tlsServerName"), tlsServerNameEdit_->text()).toString());
    intervalSpin_->setValue(settings.value(QStringLiteral("updateIntervalMs"), intervalSpin_->value()).toInt());
    SetStatus(QStringLiteral("Profile loaded"));
}

void QtViewerConnectionPanel::SaveProfile()
{
    QSettings settings(QStringLiteral("UltraVNC"), QStringLiteral("QtViewer"));
    settings.setValue(QStringLiteral("host"), hostEdit_->text());
    settings.setValue(QStringLiteral("port"), portSpin_->value());
    settings.setValue(QStringLiteral("rememberPassword"), rememberPasswordCheck_->isChecked());
    if (rememberPasswordCheck_->isChecked()) {
        settings.setValue(QStringLiteral("password"), passwordEdit_->text());
    } else {
        settings.remove(QStringLiteral("password"));
    }
    settings.setValue(QStringLiteral("shared"), sharedCheck_->isChecked());
    settings.setValue(QStringLiteral("viewOnly"), viewOnlyCheck_->isChecked());
    settings.setValue(QStringLiteral("continuousUpdates"), continuousCheck_->isChecked());
    settings.setValue(QStringLiteral("autoReconnect"), autoReconnectCheck_->isChecked());
    settings.setValue(QStringLiteral("tls"), tlsCheck_->isChecked());
    settings.setValue(QStringLiteral("tlsInsecure"), tlsInsecureCheck_->isChecked());
    settings.setValue(QStringLiteral("tlsCaFile"), tlsCaFileEdit_->text());
    settings.setValue(QStringLiteral("tlsServerName"), tlsServerNameEdit_->text());
    settings.setValue(QStringLiteral("updateIntervalMs"), intervalSpin_->value());
    settings.sync();
    SetStatus(QStringLiteral("Profile saved"));
}

void QtViewerConnectionPanel::RequestUpdate(bool showDialogOnError)
{
    const portable::ViewerConfig config = CurrentConfig();
    portable::ViewerSessionResult result;
    std::string error;
    if (!session_.Connected() && !session_.Connect(config, result, &error)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error), showDialogOnError);
        if (autoReconnectCheck_->isChecked()) {
            reconnectTimer_->start(2000);
        }
        return;
    }
    if (!session_.RequestFramebufferUpdate(false, result, &error)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error), showDialogOnError);
        if (autoReconnectCheck_->isChecked()) {
            session_.Disconnect();
            reconnectTimer_->start(2000);
        }
        return;
    }

    std::vector<unsigned int> pixels;
    if (!RawUpdateToArgbPixels(result, pixels, error) ||
        !surface_->SetArgbFramebuffer(static_cast<int>(result.update.width), static_cast<int>(result.update.height), pixels)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error.empty() ? "failed to render RFB update" : error), showDialogOnError);
        return;
    }

    if (!result.serverCutText.empty()) {
        const QString serverText = QString::fromStdString(result.serverCutText);
        serverClipboardLabel_->setText(QStringLiteral("Server clipboard: ") + serverText);
        if (QClipboard *clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(serverText);
        }
    }
    SetStatus(QString("Connected to %1:%2, %3x%4, %5 bytes")
        .arg(QString::fromStdString(config.Host()))
        .arg(config.Port())
        .arg(result.update.width)
        .arg(result.update.height)
        .arg(result.update.pixels.size()));
}

void QtViewerConnectionPanel::SendQtKeyEvent(int key, bool down)
{
    if (CurrentConfig().ViewOnly() || !session_.Connected()) {
        return;
    }
    const CARD32 keysym = QtKeyToRfbKeysym(key);
    if (keysym == 0) {
        return;
    }
    std::string error;
    if (!session_.SendKeyEvent(keysym, down, &error)) {
        StopContinuousUpdates();
        SetStatus(QStringLiteral("Error: ") + QString::fromStdString(error));
    }
}

void QtViewerConnectionPanel::SendQtPointerEvent(Qt::MouseButtons buttons, const QPoint& position)
{
    if (CurrentConfig().ViewOnly() || !session_.Connected()) {
        return;
    }
    std::string error;
    if (!session_.SendPointerEvent(QtButtonsToRfbMask(buttons),
                                   static_cast<unsigned int>(position.x()),
                                   static_cast<unsigned int>(position.y()),
                                   &error)) {
        StopContinuousUpdates();
        SetStatus(QStringLiteral("Error: ") + QString::fromStdString(error));
    }
}


void QtViewerConnectionPanel::SendClipboardText(bool showDialogOnError)
{
    if (!session_.Connected()) {
        RequestUpdate(showDialogOnError);
        if (!session_.Connected()) {
            return;
        }
    }
    std::string error;
    if (!session_.SendClientCutText(clipboardEdit_->text().toStdString(), &error)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    SetStatus(QStringLiteral("Clipboard sent"));
}


void QtViewerConnectionPanel::ListRemotePath(bool showDialogOnError)
{
    if (!session_.Connected()) {
        RequestUpdate(showDialogOnError);
        if (!session_.Connected()) return;
    }
    std::vector<portable::ViewerFileTransferEntry> entries;
    std::string error;
    if (!session_.RequestRemoteDirectory(remotePathEdit_->text().toStdString(), entries, &error)) {
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    remoteListingLabel_->setText(QStringLiteral("Remote files: %1 entries").arg(entries.size()));
    SetStatus(QStringLiteral("Remote directory listed"));
}

void QtViewerConnectionPanel::ListRemoteDrives(bool showDialogOnError)
{
    if (!session_.Connected()) {
        RequestUpdate(showDialogOnError);
        if (!session_.Connected()) return;
    }
    std::vector<portable::ViewerFileTransferEntry> entries;
    std::string error;
    if (!session_.RequestRemoteDrives(entries, &error)) {
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    remoteListingLabel_->setText(QStringLiteral("Remote roots: %1 entries").arg(entries.size()));
    SetStatus(QStringLiteral("Remote roots listed"));
}

void QtViewerConnectionPanel::DownloadRemoteFile(bool showDialogOnError)
{
    if (downloadOutputEdit_->text().isEmpty()) {
        ShowError(QStringLiteral("Download output path is required"), showDialogOnError);
        return;
    }
    if (!session_.Connected()) {
        RequestUpdate(showDialogOnError);
        if (!session_.Connected()) return;
    }
    portable::ViewerFileDownload download;
    std::string error;
    if (!session_.DownloadRemoteFile(remotePathEdit_->text().toStdString(), download, &error)) {
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    QFile out(downloadOutputEdit_->text());
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ShowError(QStringLiteral("Failed to open download output path"), showDialogOnError);
        return;
    }
    if (!download.payload.empty()) {
        out.write(reinterpret_cast<const char *>(download.payload.data()), static_cast<qint64>(download.payload.size()));
    }
    remoteListingLabel_->setText(QStringLiteral("Downloaded %1 bytes").arg(download.payload.size()));
    SetStatus(QStringLiteral("Remote file downloaded"));
}


void QtViewerConnectionPanel::UploadLocalFile(bool showDialogOnError)
{
    if (uploadInputEdit_->text().isEmpty()) {
        ShowError(QStringLiteral("Upload input path is required"), showDialogOnError);
        return;
    }
    if (!session_.Connected()) {
        RequestUpdate(showDialogOnError);
        if (!session_.Connected()) return;
    }
    QFile input(uploadInputEdit_->text());
    if (!input.open(QIODevice::ReadOnly)) {
        ShowError(QStringLiteral("Failed to open upload input path"), showDialogOnError);
        return;
    }
    const QByteArray bytes = input.readAll();
    const std::vector<CARD8> payload(bytes.begin(), bytes.end());
    std::string error;
    if (!session_.UploadRemoteFile(remotePathEdit_->text().toStdString(), payload, &error)) {
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    remoteListingLabel_->setText(QStringLiteral("Uploaded %1 bytes").arg(payload.size()));
    SetStatus(QStringLiteral("Local file uploaded"));
}

void QtViewerConnectionPanel::StartContinuousUpdatesIfRequested()
{
    const portable::ViewerConfig config = CurrentConfig();
    if (!config.ContinuousUpdates()) {
        return;
    }
    continuousTimer_->start(static_cast<int>(config.UpdateIntervalMs()));
}

void QtViewerConnectionPanel::StopContinuousUpdates()
{
    continuousTimer_->stop();
    reconnectTimer_->stop();
}

void QtViewerConnectionPanel::SetStatus(const QString& status)
{
    statusLabel_->setText(status);
}

void QtViewerConnectionPanel::ShowError(const QString& message, bool showDialog)
{
    SetStatus(QStringLiteral("Error: ") + message);
    if (showDialog) {
        QMessageBox::warning(this, QStringLiteral("UltraVNC Qt Viewer"), message);
    }
}

} // namespace qtviewer
} // namespace vncviewer
} // namespace uvnc
