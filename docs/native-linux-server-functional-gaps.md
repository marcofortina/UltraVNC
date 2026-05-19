# Native Linux server functional migration status

This document tracks Windows WinVNC server features that are not yet native Linux production features.

## Implemented in the native portable/Linux server path

- RFB SetEncodings is used for framebuffer update encoding selection.
- Raw, RRE, CoRRE, Hextile and Zlib update paths are negotiated through the portable encoder.
- PointerPos pseudo-encoding is sent to clients that request it.
- Bell and classic ServerCutText server messages are available.
- XCursor and RichCursor shape messages are encoded by the portable server path.
- Clients that advertise XCursor/RichCursor receive cursor updates after SetEncodings and when a cursor source reports a new shape.
- Native X11 cursor capture uses XFixes when available, including hotspot and alpha mask conversion.
- Cursor capture failures fall back to an explicit empty cursor update rather than a stale default arrow.
- Classic ClientCutText is parsed and can be routed to a Linux clipboard sink.
- UltraVNC extended clipboard is negotiated for clients that advertise `rfbEncodingExtendedClipboard`.
- Extended clipboard caps, notify, peek, request and provide messages are supported for UTF-8 text.
- Extended clipboard `clipProvide` payloads use the UltraVNC zlib-compressed wire format.
- A guarded X11 clipboard backend is available for local desktop sessions and can own/respond to CLIPBOARD selection requests.
- The X11 clipboard backend tracks SelectionClear, falls back from UTF8_STRING to XA_STRING, and rejects INCR transfers explicitly instead of hanging on large selections.
- UltraVNC file-transfer messages are parsed and explicitly rejected by default.
- ClientInit shared/non-shared preference is preserved in per-client state.

## Intentionally not implemented yet

### File transfer

Native Linux file transfer remains disabled by default, but the portable server now includes a guarded Linux file-transfer root, upload/download modes, directory listing, recursive listing/size, checksum responses, atomic upload, overwrite policy and command handling. Compatibility validation against real UltraVNC viewers is still intentionally left to the external matrix.

### DSM/security plugins and MSLogon

Windows DSM plugins and MSLogon are not portable Linux features. The native Linux server rejects `--security-plugin`, `--dsm-plugin` and `--mslogon` rather than pretending to load Windows-only security code.

A future Linux security design should use explicit Linux-native authentication and transport security instead of copying Windows plugin loading behavior.

### HTTP Java viewer

The Windows server ships legacy Java viewer resources and HTTP serving code. The native Linux server does not expose that path. It rejects `--http-java-viewer` so operators do not assume that the Windows-only HTTP viewer is active.

The preferred Linux direction is a real native viewer or documented external web gateway, not a silent partial Java viewer port.

## Security extensions

DSM/security plugins, MSLogon and the legacy HTTP Java viewer are explicitly rejected on native Linux. See `native-linux-security-extensions.md` for the fail-closed policy and replacement direction.

## Protocol extensions

UltraVNC scale messages and quality/compress pseudo-encoding preferences are now parsed into per-client state. Unsupported encoders remain unadvertised until they exist in the portable encoder. See `native-linux-protocol-extensions.md`.
