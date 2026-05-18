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


## Closed foundation

The WinVNC portable core milestone is closed for this incremental Linux-support
track. It provides tested protocol/update/encoding/runtime/framebuffer seams that
future Linux desktop capture and input backends must plug into.

The Memory server RFB milestone is also closed. It provides a deterministic
native Linux RFB/session harness around the portable core: handshake,
client-message processing, raw framebuffer updates, multi-update sessions and
synthetic framebuffer patterns.

The next server milestone is not more memory-server work. It is the first real
Linux framebuffer/capture source wired behind the existing portable source seam.

## Initial milestones

- Continue increasing Linux coverage for portable WinVNC encoder/update code.
- Keep hardening the platform-neutral framebuffer source abstraction.
- Add a Linux framebuffer/capture backend.
- Add a Linux input backend.
- Keep the memory-server smoke target as the closed deterministic RFB harness.
  Do not expand it into a fake desktop server; the next useful work is a real
  Linux framebuffer/capture source and then input integration.

Native Linux X11 capture progress:

- X11 capture backend selection is wired into the memory server.
- The baseline X11 path uses XGetImage when a usable DISPLAY is available.
- Optional XShm support is detected and used when built and available.
- Headless CI keeps the X11 live smoke non-fatal when DISPLAY is absent.

Native Linux PipeWire/XDG portal capture progress:

- PipeWire/XDG portal is represented as an optional capture backend.
- The current milestone provides detection and scaffolding only.
- Live PipeWire buffer import is intentionally left for the next server-capture block.

Native Linux input backend progress:

- Linux input backend selection is represented separately from capture backend selection.
- The `none` backend keeps input injection disabled.
- The optional X11/XTest backend injects keyboard and pointer events when XTest is built and available.
- Live XTest injection smoke is explicitly opt-in because it moves/injects input into the active X11 session.
- Wayland/PipeWire input injection is intentionally not treated as implemented; the current strategy is to document that limitation until a safe portal-compatible approach is selected.
