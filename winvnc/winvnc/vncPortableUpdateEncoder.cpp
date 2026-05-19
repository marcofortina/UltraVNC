// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "vncPortableUpdateEncoder.h"

#include "vncencodecorre.h"
#include "vncencodehext.h"
#include "vncencoderre.h"
#include "vncEncodeZlib.h"

namespace uvnc {
namespace winvnc {
namespace portable {

UpdateEncoder::UpdateEncoder()
    : encoder_(), initialized_(false)
{
}

bool UpdateEncoder::Initialize(const rfbPixelFormat& format, unsigned int width, unsigned int height)
{
    return Initialize(format, format, width, height);
}

bool UpdateEncoder::Initialize(const rfbPixelFormat& localFormat, const rfbPixelFormat& remoteFormat, unsigned int width, unsigned int height)
{
    rfbPixelFormat local = localFormat;
    rfbPixelFormat remote = remoteFormat;

    encoder_.SetLocalFormat(local, static_cast<int>(width), static_cast<int>(height));
    if (!encoder_.SetRemoteFormat(remote)) {
        initialized_ = false;
        return false;
    }

    initialized_ = encoder_.SetLocalFormat(local, static_cast<int>(width), static_cast<int>(height));
    return initialized_;
}

bool UpdateEncoder::EncodeRawRect(const Framebuffer& framebuffer, const rfb::Rect& rect, std::vector<BYTE>& encoded)
{
    if (!initialized_ || framebuffer.Empty() || !framebuffer.Contains(rect)) {
        return false;
    }

    encoded.assign(encoder_.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
    const UINT encodedSize = encoder_.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), encoded.data(), rect);
    if (encodedSize == 0 || encodedSize > encoded.size()) {
        encoded.clear();
        return false;
    }

    encoded.resize(encodedSize);
    return true;
}

bool UpdateEncoder::SupportsEncoding(CARD32 encoding)
{
    return encoding == rfbEncodingRaw ||
           encoding == rfbEncodingRRE ||
           encoding == rfbEncodingCoRRE ||
           encoding == rfbEncodingHextile ||
           encoding == rfbEncodingZlib;
}

bool UpdateEncoder::EncodeRect(const Framebuffer& framebuffer, const rfb::Rect& rect, CARD32 encoding, const rfbPixelFormat& remoteFormat, std::vector<BYTE>& encoded)
{
    if (!SupportsEncoding(encoding) || framebuffer.Empty() || !framebuffer.Contains(rect)) {
        return false;
    }

    rfbPixelFormat local = framebuffer.Format();
    rfbPixelFormat remote = remoteFormat;

    if (encoding == rfbEncodingRaw) {
        UpdateEncoder raw;
        return raw.Initialize(local, remote, framebuffer.Width(), framebuffer.Height()) &&
               raw.EncodeRawRect(framebuffer, rect, encoded);
    }

    if (encoding == rfbEncodingHextile) {
        vncEncodeHexT encoder;
        encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()));
        if (!encoder.SetRemoteFormat(remote) || !encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()))) {
            return false;
        }
        encoded.assign(encoder.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
        const UINT encodedSize = encoder.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), encoded.data(), rect);
        if (encodedSize == 0 || encodedSize > encoded.size()) {
            encoded.clear();
            return false;
        }
        encoded.resize(encodedSize);
        return true;
    }

    if (encoding == rfbEncodingRRE) {
        vncEncodeRRE encoder;
        encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()));
        if (!encoder.SetRemoteFormat(remote) || !encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()))) {
            return false;
        }
        encoded.assign(encoder.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
        const UINT encodedSize = encoder.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), encoded.data(), rect);
        if (encodedSize == 0 || encodedSize > encoded.size()) {
            encoded.clear();
            return false;
        }
        encoded.resize(encodedSize);
        return true;
    }

    if (encoding == rfbEncodingCoRRE) {
        vncEncodeCoRRE encoder;
        encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()));
        if (!encoder.SetRemoteFormat(remote) || !encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()))) {
            return false;
        }
        encoded.assign(encoder.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
        const UINT encodedSize = encoder.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), encoded.data(), rect);
        if (encodedSize == 0 || encodedSize > encoded.size()) {
            encoded.clear();
            return false;
        }
        encoded.resize(encodedSize);
        return true;
    }

    if (encoding == rfbEncodingZlib) {
        vncEncodeZlib encoder;
        encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()));
        if (!encoder.SetRemoteFormat(remote) || !encoder.SetLocalFormat(local, static_cast<int>(framebuffer.Width()), static_cast<int>(framebuffer.Height()))) {
            return false;
        }
        encoder.SetCompressLevel(6);
        encoded.assign(encoder.RequiredBuffSize(framebuffer.Width(), framebuffer.Height()), 0);
        const UINT encodedSize = encoder.EncodeRect(const_cast<BYTE *>(framebuffer.Data()), nullptr, encoded.data(), rect, false);
        if (encodedSize == 0 || encodedSize > encoded.size()) {
            encoded.clear();
            return false;
        }
        encoded.resize(encodedSize);
        return true;
    }

    return false;
}

} // namespace portable
} // namespace winvnc
} // namespace uvnc
