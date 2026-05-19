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
- Native file transfer UI.
- Encrypted transport/TLS security types.
- DSM/MSLogon/security plugin auth.
- Full RTF/HTML/DIB clipboard format conversion.

## Current protocol coverage

Implemented in the portable Qt viewer session path:

- RFB 3.8 no-auth handshake.
- RFB VNCAuth challenge/response when a password is provided.
- SetEncodings negotiation for raw, CopyRect, RRE, CoRRE, Hextile, Zlib, ZRLE, Tight, NewFBSize, RichCursor, XCursor, PointerPos, LastRect and ExtendedClipboard.
- Raw framebuffer update handling.
- RRE/CoRRE/Hextile/Zlib/ZRLE/Tight framebuffer update decoding, including raw, solid, packed-palette and RLE tiles.
- CopyRect/NewFBSize metadata handling.
- RichCursor/XCursor metadata and payload receive path.
- PointerPos pseudo-encoding receive path.
- LastRect pseudo-encoding handling.
- Extended clipboard receive path for UltraVNC zlib-compressed UTF-8 text.
- ClientCutText clipboard send path.
- Keyboard and pointer event forwarding over the persistent RFB session.

Still follow-up work:

- Full decoders for Tight JPEG/gradient, ZlibHex and vendor-specific compressed encodings.
- ServerCutText/extended clipboard receive UI presentation beyond internal state.
- Native file transfer UI and protocol integration.
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
  <VNC_PASSWORD>
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
  <VNC_PASSWORD>
```

Current validated portable scope covers RFB 3.8 no-auth/VNCAuth, raw updates,
CopyRect metadata, NewFBSize metadata, multi-rectangle framebuffer updates,
RichCursor/XCursor metadata, PointerPos, LastRect, ServerCutText/Bell tolerance before
updates, ExtendedClipboard UTF-8 receive, and ClientCutText send. Real-server matrix
coverage remains the final promotion gate.

## Multi-server matrix smoke

Use a pipe-separated matrix file to validate multiple real servers with one
command. Password values may be file paths; this avoids exposing secrets in the
viewer process arguments.

```sh
cmake/qt-viewer-multi-server-matrix-smoke.sh \
  docs/examples/native-linux-qt-viewer-real-server-matrix.example \
  /tmp/uvnc-multi-server-build \
  /tmp/uvnc-multi-server-install
```

Matrix columns:

```text
label|host|port|password-or-file|encodings|allow-input
```

## Production gate smoke

Run the compressed-encoding portable smoke and the configured real-server matrix in one step:

```sh
cmake/qt-viewer-production-gate-smoke.sh \
  docs/examples/native-linux-qt-viewer-real-server-matrix.example \
  /tmp/uvnc-production-gate-build \
  /tmp/uvnc-production-gate-install
```

This is still only as strong as the real servers listed in the matrix file.

### Required vendor matrix labels

Set `UVNC_VIEWER_REQUIRED_MATRIX_LABELS` to make the production gate fail when
expected real-server matrix entries are missing. Example:

```sh
UVNC_VIEWER_REQUIRED_MATRIX_LABELS=local-tigervnc,x11vnc,libvncserver,realvnc,ultravnc \
cmake/qt-viewer-production-gate-smoke.sh \
  docs/examples/native-linux-qt-viewer-real-server-matrix.example \
  /tmp/uvnc-production-gate-build \
  /tmp/uvnc-production-gate-install
```

Only enable labels for servers that are actually available in the lab; the gate
checks the matrix inputs and then executes the real-server smoke entries.

## Production baseline status

The viewer is considered production-baseline for the explicitly validated
encoding, authentication, clipboard and input paths once the local production
gate and the required real-server matrix pass. Vendor-specific extensions that
are not covered by the matrix remain outside the claim until reproduced and
covered by targeted tests.

## Portable viewer file-transfer and security-policy baseline

The native Linux viewer portable session path now has non-UI file-transfer
helpers for the protocol pieces needed by the native Linux server path:

- remote directory listing requests;
- remote drives/root listing requests;
- file download into an in-memory payload;
- file checksum request parsing.

This is intentionally protocol-level infrastructure. A visible Qt file-transfer
panel still needs separate UX work before being exposed to operators.

The portable viewer security selection is also explicit:

- VNCAuth is preferred when a password is configured and the server offers it;
- no-auth is accepted only when the viewer policy allows it;
- VeNCrypt, DSM/SecureVNC plugin and MSLogon-style UltraVNC extensions fail
  closed in the portable viewer path until a Linux-native TLS/plugin/provider
  implementation is wired into that viewer path.

Use `--disable-no-auth` for production smoke runs that must reject accidental
no-auth servers.
