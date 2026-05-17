# Native Linux porting status

This document tracks the experimental native Linux support work for UltraVNC.
It is intentionally conservative: it lists what is currently buildable on Linux
and what remains Windows-only.

## Current native Linux subset

The current CMake subset builds and tests the following components natively on Linux:

- `librdr` stream/protocol support.
- MiniLZO compression support.
- Classic VNC auth/des helper library used by portable tests.
- Experimental `uvnc_repeater_headless` executable.
- A growing WinVNC portable core slice:
  - portable region/update tracking;
  - translation tables;
  - compression helpers;
  - raw/RRE/hextile/CoRRE/zlib/zlibhex/ultra/ultra2/XZ/tight/ZRLE encoder paths;
  - portable runtime hooks for monotonic time, sleeping, process id, environment lookup
    and shutdown signalling;
  - portable framebuffer, dirty tracking, desktop-source, capture-pipeline and
    update-encoder abstractions used as the first Linux server-side seam;
  - portable TCP, RFB handshake/session/message parsing and raw framebuffer
    update serving for the experimental memory server;
  - configurable in-memory framebuffer fill byte, synthetic framebuffer patterns
    and multi-update smoke paths.

Run the local validation helper from the repository root:

```sh
cmake/native-linux-subset-smoke.sh /tmp/uvnc-linux-build /tmp/uvnc-linux-install
```

For faster WinVNC-only iterations:

```sh
cmake/winvnc-portable-core-smoke.sh /tmp/uvnc-winvnc-portable-core-build
```


## Closed native Linux milestones

The following milestones are considered closed for this incremental Linux-support
track:

- **Repeater Linux**: the headless Linux repeater target builds, validates,
  smokes loopback listeners and installs through the dedicated closure helper.
- **WinVNC portable core**: the native Linux portable WinVNC core builds and
  exercises protocol/update/encoder/runtime/framebuffer/RFB session seams through
  the dedicated closure helper.

Use `docs/native-linux-milestone-closure.md` for the exact closure scope and
validation commands.

## Still Windows-only

The following areas are not native Linux implementations yet:

- Real Linux desktop capture and input injection backends.
- Full Linux daemon/service/session integration around the portable runtime hooks.
- Native viewer UI and Windows-specific message loop integration.
- MFC/Win32 dialogs, resources, registry integration and tray UI.
- Windows socket/window compatibility layers outside the isolated portable subset.

## Porting rule

Do not pretend that Win32 UI/service/capture code can simply compile on Linux.
Move protocol, encoding and runtime-neutral code into tested portable slices first,
keep Linux runtime/capture seams explicit, then add Linux-specific implementations
for desktop capture, input, daemon/service integration and UI paths.
