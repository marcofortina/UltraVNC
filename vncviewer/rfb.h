// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// rfb.h
// This includes the rfb spec header, the port numbers,
// the CARD type definitions and various useful macros.
//

#ifndef RFB_H__
#define RFB_H__

#include <stdint.h>

// Define the CARD* types as used in X11/Xmd.h. Keep these fixed-width on
// LP64 platforms too, otherwise portable RFB wire reads can block waiting for
// 8-byte CARD32 fields on Linux.

typedef uint32_t CARD32;
typedef uint16_t CARD16;
typedef int16_t INT16;
typedef uint8_t  CARD8;

// Define the port number offsets
#define FLASH_PORT_OFFSET 5400
#define INCOMING_PORT_OFFSET 5500
#define HTTP_PORT_OFFSET 5800	// we don't use this in Venice
#define RFB_PORT_OFFSET 5900

#define PORT_TO_DISPLAY(p) ( (p) - RFB_PORT_OFFSET )
#define DISPLAY_TO_PORT(d) ( (d) + RFB_PORT_OFFSET )

#ifdef _Gii
#include <stdint.h>
#include <rfb/gii.h>
#endif

// include the protocol spec
#ifndef UVNC_RFBPROTO_INCLUDED
#define UVNC_RFBPROTO_INCLUDED
#include <rfb/rfbproto.h>
#endif

// define some quick endian conversions
// change this if necessary
#define LITTLE_ENDIAN_HOST

#ifdef LITTLE_ENDIAN_HOST

#define Swap16IfLE(s) \
    ((CARD16) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))
#define Swap32IfLE(l) \
    ((CARD32) ((((l) & 0xff000000) >> 24) | \
     (((l) & 0x00ff0000) >> 8)  | \
	 (((l) & 0x0000ff00) << 8)  | \
	 (((l) & 0x000000ff) << 24)))

#else

#define Swap16IfLE(s) (s)
#define Swap32IfLE(l) (l)

#endif


#endif
