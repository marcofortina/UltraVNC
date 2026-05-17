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
  - raw/RRE/hextile/CoRRE/zlib/zlibhex/ultra/ultra2/XZ/tight/ZRLE encoder paths.

Run the local validation helper from the repository root:

```sh
cmake/native-linux-subset-smoke.sh /tmp/uvnc-linux-build /tmp/uvnc-linux-install
```

For faster WinVNC-only iterations:

```sh
cmake/winvnc-portable-core-smoke.sh /tmp/uvnc-winvnc-portable-core-build
```

## Still Windows-only

The following areas are not native Linux implementations yet:

- WinVNC desktop capture and input injection.
- WinVNC Windows service/session handling.
- Native viewer UI and Windows-specific message loop integration.
- MFC/Win32 dialogs, resources, registry integration and tray UI.
- Windows socket/window compatibility layers outside the isolated portable subset.

## Porting rule

Do not pretend that Win32 UI/service/capture code can simply compile on Linux.
Move protocol, encoding and runtime-neutral code into tested portable slices first,
then add Linux-specific implementations for desktop capture, input, service/runtime
and UI paths.
