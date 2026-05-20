// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtServerSettingsPanel.h"

#include <QApplication>
#include <QFile>
#include <QPixmap>

#include <iostream>
#include <vector>

using uvnc::winvnc::qtserver::QtServerSettingsPanel;

namespace {

bool HasFlag(const std::vector<std::string>& args, const std::string& flag)
{
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == flag) return true;
    }
    return false;
}

std::string OptionValue(const std::vector<std::string>& args, const std::string& option)
{
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == option) return args[i + 1];
    }
    return std::string();
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    QtServerSettingsPanel panel;
    if (HasFlag(args, "--smoke-test")) {
        if (panel.GeneratedConfigText().isEmpty()) {
            std::cerr << "server settings smoke generated empty config\n";
            return 1;
        }
        return 0;
    }
    if (HasFlag(args, "--print-default-config")) {
        std::cout << panel.GeneratedConfigText().toStdString();
        return 0;
    }
    if (HasFlag(args, "--print-runtime-command")) {
        std::cout << panel.GeneratedCommandLine().toStdString() << "\n";
        return 0;
    }
    if (HasFlag(args, "--print-legacy-executable-map")) {
        std::cout << "winvnc=uvnc_winvnc_memory_server\n"
                  << "vncviewer=uvnc_qt_viewer\n"
                  << "uvnc_settings=uvnc_qt_server_settings\n"
                  << "repeater=uvnc_repeater_headless\n"
                  << "setpasswd=uvnc_winvnc_password_file\n"
                  << "createpassword=uvnc_winvnc_password_file\n"
                  << "SecureVNCPlugin.dsm=lib/ultravnc/SecureVNCPlugin.dsm\n";
        return 0;
    }
    const std::string loadConfigPath = OptionValue(args, "--load-config-print");
    if (!loadConfigPath.empty()) {
        QFile file(QString::fromStdString(loadConfigPath));
        if (!file.open(QIODevice::ReadOnly)) {
            std::cerr << "failed to open config file\n";
            return 1;
        }
        QString error;
        if (!panel.LoadConfigText(QString::fromUtf8(file.readAll()), &error)) {
            std::cerr << error.toStdString() << "\n";
            return 1;
        }
        std::cout << panel.GeneratedConfigText().toStdString();
        return 0;
    }
    if (HasFlag(args, "--print-visual-parity-report")) {
        std::cout << panel.VisualParityReport().toStdString();
        return 0;
    }
    const std::string snapshotPath = OptionValue(args, "--save-visual-parity-snapshot");
    if (!snapshotPath.empty()) {
        panel.setWindowTitle(QStringLiteral("UltraVNC Linux Server Settings"));
        panel.resize(920, 720);
        panel.show();
        QApplication::processEvents();
        if (!panel.grab().save(QString::fromStdString(snapshotPath))) {
            std::cerr << "failed to save visual parity snapshot\n";
            return 1;
        }
        return 0;
    }

    panel.setWindowTitle(QStringLiteral("UltraVNC Linux Server Settings"));
    panel.resize(920, 720);
    panel.show();
    return app.exec();
}
