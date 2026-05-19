# Native Linux server functional migration status

This document tracks Windows WinVNC server features that are not yet native Linux production features.

## Implemented in the native portable/Linux server path

- RFB SetEncodings is used for framebuffer update encoding selection.
- Raw, RRE, CoRRE, Hextile and Zlib update paths are negotiated through the portable encoder.
- PointerPos pseudo-encoding is sent to clients that request it.
- Bell and classic ServerCutText server messages are available.
- XCursor and RichCursor shape messages are encoded by the portable server path.
- Clients that advertise XCursor/RichCursor receive a cursor shape update after SetEncodings.
- Classic ClientCutText is parsed and can be routed to a Linux clipboard sink.
- A guarded X11 clipboard backend is available for local desktop sessions.
- UltraVNC file-transfer messages are parsed and explicitly rejected by default.
- ClientInit shared/non-shared preference is preserved in per-client state.

## Intentionally not implemented yet

### File transfer

Native Linux file transfer is disabled by default and `--enable-file-transfer` is rejected. The parser consumes the wire message and sends an abort response instead of silently desynchronizing the session.

A production implementation still needs:

- a server-side transfer root;
- path traversal protection;
- upload/download policy;
- quota and size limits;
- partial transfer cleanup;
- audit/log messages;
- compatibility testing against UltraVNC viewers.

### DSM/security plugins and MSLogon

Windows DSM plugins and MSLogon are not portable Linux features. The native Linux server rejects `--security-plugin`, `--dsm-plugin` and `--mslogon` rather than pretending to load Windows-only security code.

A future Linux security design should use explicit Linux-native authentication and transport security instead of copying Windows plugin loading behavior.

### HTTP Java viewer

The Windows server ships legacy Java viewer resources and HTTP serving code. The native Linux server does not expose that path. It rejects `--http-java-viewer` so operators do not assume that the Windows-only HTTP viewer is active.

The preferred Linux direction is a real native viewer or documented external web gateway, not a silent partial Java viewer port.
