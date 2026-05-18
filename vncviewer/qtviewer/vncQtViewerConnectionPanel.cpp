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
      intervalSpin_(new QSpinBox()),
      connectButton_(new QPushButton(QStringLiteral("Connect"))),
      updateButton_(new QPushButton(QStringLiteral("Update once"))),
      reconnectButton_(new QPushButton(QStringLiteral("Reconnect"))),
      disconnectButton_(new QPushButton(QStringLiteral("Disconnect"))),
      statusLabel_(new QLabel(QStringLiteral("Disconnected"))),
      surface_(new QtViewerSurface()),
      continuousTimer_(new QTimer(this))
{
    hostEdit_->setObjectName(QStringLiteral("hostEdit"));
    portSpin_->setObjectName(QStringLiteral("portSpin"));
    passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
    sharedCheck_->setObjectName(QStringLiteral("sharedCheck"));
    viewOnlyCheck_->setObjectName(QStringLiteral("viewOnlyCheck"));
    continuousCheck_->setObjectName(QStringLiteral("continuousCheck"));
    intervalSpin_->setObjectName(QStringLiteral("intervalSpin"));
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));

    portSpin_->setRange(1, 65535);
    portSpin_->setValue(initialConfig.Port());
    passwordEdit_->setEchoMode(QLineEdit::Password);
    sharedCheck_->setChecked(initialConfig.Shared());
    viewOnlyCheck_->setChecked(initialConfig.ViewOnly());
    continuousCheck_->setChecked(initialConfig.ContinuousUpdates());
    intervalSpin_->setRange(100, 60000);
    intervalSpin_->setValue(static_cast<int>(initialConfig.UpdateIntervalMs()));
    continuousTimer_->setSingleShot(false);

    QFormLayout *form = new QFormLayout();
    form->addRow(QStringLiteral("Host"), hostEdit_);
    form->addRow(QStringLiteral("Port"), portSpin_);
    form->addRow(QStringLiteral("Password"), passwordEdit_);
    form->addRow(QStringLiteral("Update interval ms"), intervalSpin_);

    QHBoxLayout *options = new QHBoxLayout();
    options->addWidget(sharedCheck_);
    options->addWidget(viewOnlyCheck_);
    options->addWidget(continuousCheck_);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(connectButton_);
    buttons->addWidget(updateButton_);
    buttons->addWidget(reconnectButton_);
    buttons->addWidget(disconnectButton_);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(options);
    layout->addLayout(buttons);
    layout->addWidget(statusLabel_);
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
    QObject::connect(continuousTimer_, &QTimer::timeout, this, [this]() {
        RequestUpdate(false);
    });
    surface_->SetKeyEventCallback([this](int key, bool down) {
        SendQtKeyEvent(key, down);
    });
    surface_->SetPointerEventCallback([this](Qt::MouseButtons buttons, const QPoint& position) {
        SendQtPointerEvent(buttons, position);
    });

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

void QtViewerConnectionPanel::RequestUpdate(bool showDialogOnError)
{
    const portable::ViewerConfig config = CurrentConfig();
    portable::ViewerSessionResult result;
    std::string error;
    if (!session_.Connected() && !session_.Connect(config, result, &error)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }
    if (!session_.RequestFramebufferUpdate(false, result, &error)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error), showDialogOnError);
        return;
    }

    std::vector<unsigned int> pixels;
    if (!RawUpdateToArgbPixels(result, pixels, error) ||
        !surface_->SetArgbFramebuffer(static_cast<int>(result.update.width), static_cast<int>(result.update.height), pixels)) {
        StopContinuousUpdates();
        ShowError(QString::fromStdString(error.empty() ? "failed to render RFB update" : error), showDialogOnError);
        return;
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
