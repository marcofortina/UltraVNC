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
#include <QFormLayout>
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
      intervalSpin_(new QSpinBox()),
      connectButton_(new QPushButton(QStringLiteral("Connect"))),
      updateButton_(new QPushButton(QStringLiteral("Update once"))),
      reconnectButton_(new QPushButton(QStringLiteral("Reconnect"))),
      disconnectButton_(new QPushButton(QStringLiteral("Disconnect"))),
      loadProfileButton_(new QPushButton(QStringLiteral("Load profile"))),
      saveProfileButton_(new QPushButton(QStringLiteral("Save profile"))),
      clipboardEdit_(new QLineEdit()),
      sendClipboardButton_(new QPushButton(QStringLiteral("Send clipboard"))),
      statusLabel_(new QLabel(QStringLiteral("Disconnected"))),
      serverClipboardLabel_(new QLabel(QStringLiteral("Server clipboard: <none>"))),
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
    intervalSpin_->setObjectName(QStringLiteral("intervalSpin"));
    clipboardEdit_->setObjectName(QStringLiteral("clipboardEdit"));
    sendClipboardButton_->setObjectName(QStringLiteral("sendClipboardButton"));
    loadProfileButton_->setObjectName(QStringLiteral("loadProfileButton"));
    saveProfileButton_->setObjectName(QStringLiteral("saveProfileButton"));
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    serverClipboardLabel_->setObjectName(QStringLiteral("serverClipboardLabel"));

    portSpin_->setRange(1, 65535);
    portSpin_->setValue(initialConfig.Port());
    passwordEdit_->setEchoMode(QLineEdit::Password);
    sharedCheck_->setChecked(initialConfig.Shared());
    viewOnlyCheck_->setChecked(initialConfig.ViewOnly());
    continuousCheck_->setChecked(initialConfig.ContinuousUpdates());
    intervalSpin_->setRange(100, 60000);
    intervalSpin_->setValue(static_cast<int>(initialConfig.UpdateIntervalMs()));
    continuousTimer_->setSingleShot(false);
    reconnectTimer_->setSingleShot(true);

    QFormLayout *form = new QFormLayout();
    form->addRow(QStringLiteral("Host"), hostEdit_);
    form->addRow(QStringLiteral("Port"), portSpin_);
    form->addRow(QStringLiteral("Password"), passwordEdit_);
    form->addRow(QStringLiteral("Update interval ms"), intervalSpin_);

    QHBoxLayout *options = new QHBoxLayout();
    options->addWidget(sharedCheck_);
    options->addWidget(viewOnlyCheck_);
    options->addWidget(continuousCheck_);
    options->addWidget(autoReconnectCheck_);

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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(options);
    layout->addLayout(buttons);
    layout->addLayout(profiles);
    layout->addLayout(clipboard);
    layout->addWidget(statusLabel_);
    layout->addWidget(serverClipboardLabel_);
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
    return config;
}


void QtViewerConnectionPanel::LoadProfile()
{
    QSettings settings(QStringLiteral("UltraVNC"), QStringLiteral("QtViewer"));
    hostEdit_->setText(settings.value(QStringLiteral("host"), hostEdit_->text()).toString());
    portSpin_->setValue(settings.value(QStringLiteral("port"), portSpin_->value()).toInt());
    passwordEdit_->setText(settings.value(QStringLiteral("password"), passwordEdit_->text()).toString());
    sharedCheck_->setChecked(settings.value(QStringLiteral("shared"), sharedCheck_->isChecked()).toBool());
    viewOnlyCheck_->setChecked(settings.value(QStringLiteral("viewOnly"), viewOnlyCheck_->isChecked()).toBool());
    continuousCheck_->setChecked(settings.value(QStringLiteral("continuousUpdates"), continuousCheck_->isChecked()).toBool());
    autoReconnectCheck_->setChecked(settings.value(QStringLiteral("autoReconnect"), autoReconnectCheck_->isChecked()).toBool());
    intervalSpin_->setValue(settings.value(QStringLiteral("updateIntervalMs"), intervalSpin_->value()).toInt());
    SetStatus(QStringLiteral("Profile loaded"));
}

void QtViewerConnectionPanel::SaveProfile()
{
    QSettings settings(QStringLiteral("UltraVNC"), QStringLiteral("QtViewer"));
    settings.setValue(QStringLiteral("host"), hostEdit_->text());
    settings.setValue(QStringLiteral("port"), portSpin_->value());
    settings.setValue(QStringLiteral("password"), passwordEdit_->text());
    settings.setValue(QStringLiteral("shared"), sharedCheck_->isChecked());
    settings.setValue(QStringLiteral("viewOnly"), viewOnlyCheck_->isChecked());
    settings.setValue(QStringLiteral("continuousUpdates"), continuousCheck_->isChecked());
    settings.setValue(QStringLiteral("autoReconnect"), autoReconnectCheck_->isChecked());
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
        serverClipboardLabel_->setText(QStringLiteral("Server clipboard: ") + QString::fromStdString(result.serverCutText));
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
