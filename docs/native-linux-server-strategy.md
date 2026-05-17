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
- multiple WinVNC encoder paths.

This is not a Linux server yet. It is the necessary foundation for one.

## Initial milestones

- Continue increasing Linux coverage for portable WinVNC encoder/update code.
- Add a platform-neutral framebuffer source abstraction.
- Add a Linux framebuffer/capture backend.
- Add a Linux input backend.
- Add a headless Linux server smoke target before any desktop UI integration.
