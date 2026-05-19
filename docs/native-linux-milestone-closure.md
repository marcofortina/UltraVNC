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
  /tmp/uvnc-linux-closure-install \
  /tmp/uvnc-qt-closure-builds
```

## CI coverage

The `.github/workflows/native-linux-subset.yml` workflow runs the aggregate
CI-safe gate:

```sh
cmake/native-linux-ci-gate-smoke.sh \
  /tmp/uvnc-native-linux-ci-gate-build \
  /tmp/uvnc-native-linux-ci-gate-install
```

The gate includes the closure helpers plus Linux server real-runtime checks in
CI-safe mode, negative runtime checks, non-invasive user-service checks and the
Qt viewer compressed-encoding smoke.

Live X11 server runtime validation and live systemd user-service validation remain explicit manual gates. The external VNC client/server compatibility matrix now has reproducible checker/runner helpers, but rows that require GUI/licensed/Windows clients still require real operator evidence.
PR creation and GitHub check inspection are intentionally deferred until the end
of the current native Linux hardening pass.


## Final readiness helper

Use this only when preparing the final issue/PR evidence bundle. It fails if the
external viewer matrix file is missing:

```bash
cmake/native-linux-final-readiness-smoke.sh \
  /tmp/uvnc-native-linux-final-build \
  /tmp/uvnc-native-linux-final-install \
  docs/examples/native-linux-server-external-viewer-matrix.example
```

Do not mark TigerVNC, LibVNC, RealVNC or Windows UltraVNC compatibility as closed
until the matrix rows are either automated (`ready`) or backed by manual evidence.
