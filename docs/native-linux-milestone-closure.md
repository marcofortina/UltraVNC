# Native Linux milestone closure notes

This document defines the currently closed native Linux milestones and the
validation required before they are presented as completed work.

## Closed milestone: Repeater Linux

Scope:

- native Linux `uvnc_repeater_headless` CMake target;
- Linux compatibility shim isolated under `cmake/repeater_linux_compat`;
- mode/config validation;
- loopback smoke coverage;
- install smoke coverage;
- documented runtime options and config-file flow.

Validation:

```sh
cmake/repeater-linux-smoke.sh /tmp/uvnc-repeater-linux-build /tmp/uvnc-linux-closure-install
```

This does not mean the Windows GUI repeater has been ported. It means the
headless Linux repeater milestone is closed and has a dedicated closure smoke.

## Closed milestone: WinVNC portable core

Scope:

- portable regions/update tracking;
- translation/compression helpers;
- encoder coverage for the native Linux subset;
- portable runtime/shutdown/TCP seams;
- framebuffer, dirty tracking, desktop-source and capture-pipeline abstractions;
- RFB handshake, client message parsing and raw framebuffer update serving;
- experimental memory-server executable used to exercise the portable core.

Validation:

```sh
cmake/winvnc-portable-core-closure.sh /tmp/uvnc-winvnc-portable-core-build /tmp/uvnc-linux-closure-install
```

This does not mean a production Linux desktop server exists. Real Linux desktop
capture, input injection and daemon/session integration remain future milestones.

## Combined closure smoke

```sh
cmake/native-linux-closure-smoke.sh \
  /tmp/uvnc-repeater-linux-build \
  /tmp/uvnc-winvnc-portable-core-build \
  /tmp/uvnc-linux-closure-install
```
