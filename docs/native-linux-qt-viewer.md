# Native Linux Qt viewer shell

This document tracks the first native Linux viewer milestone for #348.

The initial native Linux viewer frontend is Qt-based. It is intentionally added
as a separate shell instead of trying to compile the existing Win32 viewer UI on
Linux through broad compatibility shims.

## Ubuntu packages

```sh
sudo apt install -y qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

`qt6-base-dev` provides Qt6 Widgets headers/libraries. `qt6-base-dev-tools`
provides the usual Qt helper tools, and `libgl1-mesa-dev` covers the OpenGL
headers/libraries commonly required by Qt desktop builds on Ubuntu.

## Build

The target is optional and is built only when Qt6 Widgets is available and the
native Qt viewer option is enabled:

```sh
cmake -S cmake -B /tmp/uvnc-qt-viewer-build -G Ninja \
  -DULTRAVNC_BUILD_PORTABLE_LIBS=ON \
  -DULTRAVNC_BUILD_WINDOWS_APPS=OFF \
  -DULTRAVNC_BUILD_QT_VIEWER=ON
cmake --build /tmp/uvnc-qt-viewer-build --target uvnc_qt_viewer
```

## Smoke

```sh
cmake/qt-viewer-smoke.sh \
  /tmp/uvnc-qt-viewer-build \
  /tmp/uvnc-qt-viewer-install
```

The smoke uses `QT_QPA_PLATFORM=offscreen` so CI can validate shell creation
without requiring a visible desktop session.

## RFB session smoke

```sh
cmake/qt-viewer-rfb-smoke.sh   /tmp/uvnc-qt-viewer-rfb-build   /tmp/uvnc-qt-viewer-rfb-install
```

This helper starts the native Linux memory server on a loopback port, runs
`uvnc_qt_viewer --connect-update-smoke`, completes the RFB handshake and reads
one raw framebuffer update. It also runs `--connect-display-smoke` to render that
update into the Qt framebuffer surface with `QT_QPA_PLATFORM=offscreen`.

## Persistent input smoke

```sh
cmake/qt-viewer-persistent-input-smoke.sh   /tmp/uvnc-qt-viewer-persistent-input-build   /tmp/uvnc-qt-viewer-persistent-input-install
```

This helper keeps one RFB connection open long enough to send keyboard and
pointer events before requesting a framebuffer update. It exercises the portable
persistent viewer session and the same wire encoders used by the Qt input
forwarding path.

## Manual validation against a known VNC server

Use this only against a no-auth test server. The current native Linux viewer
smoke client intentionally supports only RFB 3.8 no-auth plus raw framebuffer
updates.

```sh
cmake/qt-viewer-known-server-smoke.sh \
  127.0.0.1 \
  5900 \
  /tmp/uvnc-qt-viewer-known-server-build \
  /tmp/uvnc-qt-viewer-known-server-install
```

Equivalent direct commands after building/installing `uvnc_qt_viewer`:

```sh
/tmp/uvnc-qt-viewer-known-server-install/bin/uvnc_qt_viewer \
  --host 127.0.0.1 \
  --port 5900 \
  --view-only \
  --connect-update-smoke

QT_QPA_PLATFORM=offscreen \
/tmp/uvnc-qt-viewer-known-server-install/bin/uvnc_qt_viewer \
  --host 127.0.0.1 \
  --port 5900 \
  --view-only \
  --connect-display-smoke
```

Expected output contains the negotiated framebuffer size, desktop name and raw
update byte count.

## Current scope

Implemented in this milestone:

- Qt6 Widgets shell target: `uvnc_qt_viewer`.
- Portable viewer configuration parsing.
- CLI validation and smoke mode.
- Offscreen Qt event-loop smoke.
- Qt framebuffer surface widget with deterministic synthetic pixels.
- Local keyboard and pointer event handling inside the Qt surface.
- Interactive connection panel for host, port, password, shared/view-only mode and continuous-update polling.
- Persistent RFB session ownership inside the Qt connection panel.
- RFB handshake/update smoke path against `uvnc_winvnc_memory_server`.
- One-shot RFB update rendering into the Qt framebuffer surface.
- Manual known-server validation helper for no-auth raw RFB test servers.
- Keyboard/pointer forwarding path for supported Qt key/button events.

Not implemented in this milestone:

- Continuous live remote framebuffer updates against broad real-world VNC servers.
- Clipboard and file transfer.

## Current protocol coverage

Implemented in the portable Qt viewer session path:

- RFB 3.8 no-auth handshake.
- RFB VNCAuth challenge/response when a password is provided.
- SetEncodings negotiation for raw, CopyRect and NewFBSize.
- Raw framebuffer update handling.
- CopyRect/NewFBSize metadata handling.
- ClientCutText clipboard send path.
- Keyboard and pointer event forwarding over the persistent RFB session.

Still follow-up work:

- Full decoders for compressed encodings such as Hextile/Zlib/ZRLE/Tight.
- ServerCutText/extended clipboard receive UI.
- Real-server interoperability matrix across multiple VNC servers.

Known-server smoke can also pass a VNCAuth password as the fifth argument:

```sh
cmake/qt-viewer-known-server-smoke.sh 127.0.0.1 5900 \
  /tmp/uvnc-known-server-build \
  /tmp/uvnc-known-server-install \
  secret
```

Real-server update compatibility note:

The portable Qt viewer now accepts FramebufferUpdate messages with multiple rectangles and composes supported raw/CopyRect rectangles into the local framebuffer before handing pixels to the Qt surface. This is required for interoperability with real VNC servers, which commonly send more than one rectangle per update.

## Real-server matrix smoke

Use the matrix helper for a real VNC server validation pass. It runs the
handshake/update smoke and the offscreen Qt display smoke with raw/CopyRect/NewFBSize
encoding preferences and optional VNCAuth password.

```sh
cmake/qt-viewer-real-server-matrix-smoke.sh \
  127.0.0.1 \
  5901 \
  /tmp/uvnc-real-server-build \
  /tmp/uvnc-real-server-install \
  secret
```

The input/clipboard phase sends key, pointer and ClientCutText messages, so it is
opt-in for real desktops:

```sh
UVNC_VIEWER_REAL_SERVER_ALLOW_INPUT=1 \
cmake/qt-viewer-real-server-matrix-smoke.sh \
  127.0.0.1 \
  5901 \
  /tmp/uvnc-real-server-build \
  /tmp/uvnc-real-server-install \
  secret
```

Current validated real-server scope covers RFB 3.8 no-auth/VNCAuth, raw updates,
CopyRect metadata, NewFBSize metadata, multi-rectangle framebuffer updates,
ServerCutText/Bell tolerance before updates, and ClientCutText send.
