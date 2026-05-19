// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncQtServerSettingsPanel.h"

#include <QApplication>

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

    panel.setWindowTitle(QStringLiteral("UltraVNC Linux Server Settings"));
    panel.resize(920, 720);
    panel.show();
    return app.exec();
}
