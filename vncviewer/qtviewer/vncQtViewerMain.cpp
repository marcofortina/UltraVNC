// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"
#include "vncPortableViewerSession.h"
#include "vncQtViewerConnectionPanel.h"
#include "vncQtViewerSurface.h"

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>
#include <string>
#include <vector>

using uvnc::vncviewer::portable::ParseViewerCli;
using uvnc::vncviewer::portable::PersistentViewerSession;
using uvnc::vncviewer::portable::ViewerCliOptions;
using uvnc::vncviewer::portable::ViewerCliUsage;
using uvnc::vncviewer::portable::ViewerSession;
using uvnc::vncviewer::portable::ViewerSessionResult;
using uvnc::vncviewer::qtviewer::QtViewerConnectionPanel;
using uvnc::vncviewer::qtviewer::QtViewerSurface;

namespace {

std::vector<std::string> ArgsToVector(int argc, char **argv)
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return args;
}

QWidget *CreateViewerWindow(const ViewerCliOptions& options)
{
    QWidget *window = new QWidget();
    window->setWindowTitle(QStringLiteral("UltraVNC Qt Viewer (experimental Linux)"));

    QVBoxLayout *layout = new QVBoxLayout(window);
    QLabel *title = new QLabel(QStringLiteral("UltraVNC Qt Viewer"));
    QLabel *status = new QLabel(QStringLiteral("Experimental native Linux Qt shell"));
    QtViewerConnectionPanel *panel = new QtViewerConnectionPanel(options.config);

    layout->addWidget(title);
    layout->addWidget(status);
    layout->addWidget(panel, 1);
    window->setLayout(layout);
    window->resize(760, 520);
    return window;
}


unsigned int ScaleColor(unsigned int value, unsigned int max)
{
    return max == 0 ? 0 : (value * 255u) / max;
}

bool RawUpdateToArgbPixels(const ViewerSessionResult& result, std::vector<unsigned int>& pixels, std::string& error)
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

int RunConnectDisplaySmoke(int argc, char **argv, const ViewerCliOptions& options)
{
    ViewerSessionResult result;
    std::string error;
    if (!ViewerSession().RequestOneFramebufferUpdate(options.config, result, &error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::vector<unsigned int> pixels;
    if (!RawUpdateToArgbPixels(result, pixels, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle(QStringLiteral("UltraVNC Qt Viewer RFB display smoke"));
    QVBoxLayout *layout = new QVBoxLayout(&window);
    QLabel *status = new QLabel(QString("RFB update: %1x%2 from %3")
        .arg(result.update.width)
        .arg(result.update.height)
        .arg(QString::fromStdString(result.desktopName)));
    QtViewerSurface *surface = new QtViewerSurface();
    if (!surface->SetArgbFramebuffer(static_cast<int>(result.update.width), static_cast<int>(result.update.height), pixels)) {
        std::cerr << "failed to load RFB update into Qt surface\n";
        return 1;
    }
    layout->addWidget(status);
    layout->addWidget(surface, 1);
    window.resize(640, 360);
    window.show();
    QTimer::singleShot(0, &app, &QCoreApplication::quit);

    std::cout << "displayed " << result.update.width << "x" << result.update.height
              << " name=\"" << result.desktopName << "\""
              << " bytes=" << result.update.pixels.size() << "\n";
    return app.exec();
}

int RunConnectSmoke(const ViewerCliOptions& options)
{
    ViewerSessionResult result;
    std::string error;
    const bool ok = options.connectUpdateSmoke ?
        ViewerSession().RequestOneFramebufferUpdate(options.config, result, &error) :
        ViewerSession().RunHandshake(options.config, result, &error);
    if (!ok) {
        std::cerr << error << "\n";
        return 1;
    }
    std::cout << "connected " << result.width << "x" << result.height
              << " name=\"" << result.desktopName << "\"";
    if (result.update.received) {
        std::cout << " update=" << result.update.width << "x" << result.update.height
                  << " bytes=" << result.update.pixels.size();
    }
    std::cout << "\n";
    return 0;
}

int RunPersistentInputSmoke(const ViewerCliOptions& options)
{
    PersistentViewerSession session;
    ViewerSessionResult result;
    std::string error;
    if (!session.Connect(options.config, result, &error)) {
        std::cerr << error << "\n";
        return 1;
    }
    if (!session.SendKeyEvent(0xff0d, true, &error) ||
        !session.SendKeyEvent(0xff0d, false, &error) ||
        !session.SendPointerEvent(1, 3, 4, &error) ||
        !session.SendClientCutText(options.clipboardText, &error) ||
        !session.RequestFramebufferUpdate(false, result, &error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::cout << "persistent-input " << result.width << "x" << result.height
              << " name=\"" << result.desktopName << "\""
              << " update=" << result.update.width << "x" << result.update.height
              << " bytes=" << result.update.pixels.size() << "\n";
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    ViewerCliOptions options;
    std::string error;
    if (!ParseViewerCli(ArgsToVector(argc, argv), options, error)) {
        std::cerr << error << "\n";
        return 2;
    }

    if (options.help) {
        std::cout << ViewerCliUsage(argv[0]);
        return 0;
    }

    if (options.validateOnly) {
        return 0;
    }

    if (options.connectDisplaySmoke) {
        return RunConnectDisplaySmoke(argc, argv, options);
    }

    if (options.persistentInputSmoke) {
        return RunPersistentInputSmoke(options);
    }

    if (options.connectSmoke || options.connectUpdateSmoke) {
        return RunConnectSmoke(options);
    }

    QApplication app(argc, argv);
    QWidget *window = CreateViewerWindow(options);
    window->show();

    if (options.smokeTest) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
