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


## Closed milestone: Memory server RFB

Scope:

- experimental native Linux `uvnc_winvnc_memory_server` executable;
- RFB 3.8 no-auth handshake for local smoke validation;
- server-init metadata for configurable framebuffer size/name;
- client message handling for set-pixel-format, set-encodings, key events,
  pointer events, client cut text and framebuffer update requests;
- raw framebuffer update responses, clipped/empty update handling and incremental
  empty updates when the synthetic framebuffer is unchanged;
- configurable synthetic framebuffer patterns: `solid`, `checker`, `gradient-x`
  and `gradient-y`;
- single-client handshake, one-update and multi-update smoke coverage;
- installed-binary smoke coverage.

Validation:

```sh
cmake/memory-server-rfb-closure.sh /tmp/uvnc-memory-server-rfb-build /tmp/uvnc-linux-closure-install
```

This does not mean a real Linux desktop server exists. It means the native Linux
RFB memory-server milestone is closed as a deterministic protocol/session test
harness for the future real Linux framebuffer and input backends.

## Combined closure smoke

```sh
cmake/native-linux-closure-smoke.sh \
  /tmp/uvnc-repeater-linux-build \
  /tmp/uvnc-winvnc-portable-core-build \
  /tmp/uvnc-linux-closure-install
```

## CI coverage

The `.github/workflows/native-linux-subset.yml` workflow runs the closure helpers
on `main`/`master` pushes and pull requests. Feature branches can still run the
same helpers locally before opening a PR.
