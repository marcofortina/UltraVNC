# Native Linux server strategy

The existing WinVNC server is a Windows desktop server. A native Linux server
must not be presented as available until the platform-specific capture, input,
service and session layers exist.

## Strategy

1. Keep protocol/update/encoding logic portable and tested first.
2. Isolate Windows desktop capture from framebuffer/update production.
3. Isolate Windows input injection from RFB pointer/keyboard event handling.
4. Isolate Windows service/session/tray behavior from server lifetime management.
5. Add Linux-specific implementations for desktop capture and input as explicit
   platform code, not hidden inside generic Win32 compatibility shims.

## Current server-side progress

The native Linux subset already builds and tests a server-side portable core slice:

- regions and update tracking;
- translation helpers;
- compression helpers;
- multiple WinVNC encoder paths;
- portable TCP/RFB handshake and client-message processing;
- experimental `uvnc_winvnc_memory_server`, which serves a synthetic
  in-memory framebuffer, configurable synthetic patterns and raw framebuffer updates.

This is not a production Linux desktop server yet. It is now a real native
Linux server-side skeleton, but it still lacks real desktop capture and input
integration.

## Initial milestones

- Continue increasing Linux coverage for portable WinVNC encoder/update code.
- Keep hardening the platform-neutral framebuffer source abstraction.
- Add a Linux framebuffer/capture backend.
- Add a Linux input backend.
- Promote the memory-server smoke target only after it can exercise real Linux
  capture/input backends. The pattern mode is intentionally synthetic: it keeps
  protocol/session tests deterministic while the real capture backend is still
  being isolated.
