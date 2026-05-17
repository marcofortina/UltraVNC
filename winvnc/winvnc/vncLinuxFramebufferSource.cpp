// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncLinuxFramebufferSource.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace uvnc {
namespace winvnc {
namespace linuxfb {
namespace {

void SetError(std::string *error, const std::string& message)
{
    if (error) {
        *error = message;
    }
}

bool ReadExactFile(int fd, BYTE *buffer, std::size_t length)
{
    std::size_t offset = 0;
    while (offset < length) {
        const ssize_t got = read(fd, buffer + offset, length - offset);
        if (got == 0) {
            return false;
        }
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        offset += static_cast<std::size_t>(got);
    }
    return true;
}

} // namespace

bool LoadRawFramebufferFile(const std::string& path,
                            unsigned int width,
                            unsigned int height,
                            const rfbPixelFormat& format,
                            portable::Framebuffer& framebuffer,
                            std::string *error)
{
    if (path.empty()) {
        SetError(error, "raw framebuffer path must not be empty");
        return false;
    }

    portable::Framebuffer loaded;
    if (!loaded.Reset(width, height, format)) {
        SetError(error, "invalid raw framebuffer dimensions or format");
        return false;
    }

    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        SetError(error, "cannot stat raw framebuffer file");
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        SetError(error, "raw framebuffer path is not a regular file");
        return false;
    }
    if (static_cast<std::size_t>(st.st_size) != loaded.SizeBytes()) {
        SetError(error, "raw framebuffer file size does not match dimensions and pixel format");
        return false;
    }

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        SetError(error, "cannot open raw framebuffer file");
        return false;
    }

    const bool ok = ReadExactFile(fd, loaded.Data(), loaded.SizeBytes());
    close(fd);
    if (!ok) {
        SetError(error, "cannot read raw framebuffer file");
        return false;
    }

    framebuffer = loaded;
    if (error) {
        error->clear();
    }
    return true;
}

RawFileDesktopSource::RawFileDesktopSource(const std::string& path,
                                           unsigned int width,
                                           unsigned int height,
                                           const rfbPixelFormat& format)
    : path_(path),
      width_(width),
      height_(height),
      format_(format)
{
}

rfb::Rect RawFileDesktopSource::Size() const
{
    return rfb::Rect(0, 0, static_cast<int>(width_), static_cast<int>(height_));
}

rfbPixelFormat RawFileDesktopSource::Format() const
{
    return format_;
}

bool RawFileDesktopSource::Snapshot(portable::Framebuffer& destination, rfb::Region2D& changed)
{
    if (!LoadRawFramebufferFile(path_, width_, height_, format_, destination)) {
        return false;
    }
    changed.reset(destination.Bounds());
    return true;
}

} // namespace linuxfb
} // namespace winvnc
} // namespace uvnc
