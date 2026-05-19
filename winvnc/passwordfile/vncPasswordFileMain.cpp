// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void Usage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << " --output <path> --password <value> [--force]\n"
              << "       " << argv0 << " --validate --output <path>\n";
}

bool IsPrivateRegularFile(const std::string& path, std::string& error)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        error = std::string("cannot stat password file: ") + std::strerror(errno);
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        error = "password file is not a regular file";
        return false;
    }
    if (st.st_mode & (S_IRWXG | S_IRWXO)) {
        error = "password file must not be accessible by group/other";
        return false;
    }
    return true;
}

bool ValidatePasswordValue(const std::string& password, std::string& error)
{
    if (password.empty()) {
        error = "password must not be empty";
        return false;
    }
    if (password.size() > 8) {
        error = "VNCAuth password must be at most 8 bytes";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    std::string output;
    std::string password;
    bool force = false;
    bool validateOnly = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            Usage(argv[0]);
            return 0;
        } else if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--password" && i + 1 < argc) {
            password = argv[++i];
        } else if (arg == "--force") {
            force = true;
        } else if (arg == "--validate") {
            validateOnly = true;
        } else {
            std::cerr << "invalid argument: " << arg << "\n";
            Usage(argv[0]);
            return 2;
        }
    }

    if (output.empty()) {
        std::cerr << "--output is required\n";
        return 2;
    }

    std::string error;
    if (validateOnly) {
        if (!IsPrivateRegularFile(output, error)) {
            std::cerr << error << "\n";
            return 1;
        }
        std::ifstream input(output.c_str(), std::ios::binary);
        std::string value;
        std::getline(input, value);
        if (!ValidatePasswordValue(value, error)) {
            std::cerr << error << "\n";
            return 1;
        }
        return 0;
    }

    if (!ValidatePasswordValue(password, error)) {
        std::cerr << error << "\n";
        return 1;
    }
    if (!force && access(output.c_str(), F_OK) == 0) {
        std::cerr << "refusing to overwrite existing password file without --force\n";
        return 1;
    }

    std::ofstream out(output.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open output password file\n";
        return 1;
    }
    out << password << "\n";
    out.close();
    if (!out) {
        std::cerr << "cannot write output password file\n";
        return 1;
    }
    if (chmod(output.c_str(), S_IRUSR | S_IWUSR) != 0) {
        std::cerr << "cannot chmod password file: " << std::strerror(errno) << "\n";
        return 1;
    }
    return 0;
}
