// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableViewerCli.h"
#include "vncQtViewerSurface.h"

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>
#include <vector>
#include <string>
#include <vector>

using uvnc::vncviewer::portable::ParseViewerCli;
using uvnc::vncviewer::portable::ViewerCliOptions;
using uvnc::vncviewer::portable::ViewerCliUsage;
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
    QLabel *target = new QLabel(QString("Target: %1:%2")
        .arg(QString::fromStdString(options.config.Host()))
        .arg(options.config.Port()));
    QLabel *mode = new QLabel(options.config.ViewOnly()
        ? QStringLiteral("Mode: view-only")
        : QStringLiteral("Mode: input-capable shell"));
    QtViewerSurface *surface = new QtViewerSurface();
    std::vector<unsigned int> pixels(160 * 90, 0xff1f2937u);
    for (int y = 0; y < 90; ++y) {
        for (int x = 0; x < 160; ++x) {
            const unsigned int shade = static_cast<unsigned int>((x * 255) / 159);
            pixels[static_cast<std::size_t>(y * 160 + x)] = 0xff000000u | (shade << 16) | (0x66u << 8) | 0xccu;
        }
    }
    surface->SetArgbFramebuffer(160, 90, pixels);

    layout->addWidget(title);
    layout->addWidget(status);
    layout->addWidget(target);
    layout->addWidget(mode);
    layout->addWidget(surface, 1);
    window->setLayout(layout);
    window->resize(640, 360);
    return window;
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

    QApplication app(argc, argv);
    QWidget *window = CreateViewerWindow(options);
    window->show();

    if (options.smokeTest) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
