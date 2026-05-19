## Production baseline status

The native Linux Qt viewer has a production-baseline validation path when the
local compressed-encoding smoke, the Qt viewer smoke helpers and the configured
real-server matrix all pass. The production gate does not replace real vendor
coverage; it makes missing matrix entries explicit and reproducible.

# Native Linux Qt viewer production-readiness checklist

The native Linux Qt viewer is now a real executable that can connect to VNCAuth-capable VNC servers and render framebuffer updates through the Qt surface.

Production-ready scope currently covered:

- interactive host/port/password/options UI;
- VeNCrypt/TLS viewer controls for CA file, server name and lab-only insecure mode;
- Qt file-transfer controls for remote listing, roots, download and upload through the portable session API;
- profile load/save through Qt settings;
- reconnect/disconnect/update controls;
- optional auto reconnect;
- VNCAuth security type 2 challenge/response;
- raw, CopyRect, RRE, CoRRE, Hextile, Zlib, ZRLE, Tight and NewFBSize negotiation/handling; ZRLE covers raw, solid, packed-palette and RLE tiles;
- multi-rectangle FramebufferUpdate handling;
- ClientCutText clipboard send path;
- portable file-transfer helpers for directory listing, roots, download, upload and checksums;
- ServerCutText receive display;
- UltraVNC ExtendedClipboard UTF-8 receive path;
- RichCursor/XCursor/PointerPos/LastRect pseudo-encoding handling;
- Bell tolerance;
- keyboard/pointer forwarding path;
- desktop entry/icon/docs install artifacts;
- loopback, portable and real-server smoke helpers.

Known limits still outside the current production baseline:

- compressed encoding decoders not yet implemented for Tight JPEG/gradient, ZlibHex and vendor-specific extensions;
- MSLogonII viewer compatibility is implemented for original UltraVNC servers when explicitly configured; DSM/SecureVNC plugin stream transforms remain explicit fail-closed legacy extension requests until a native provider ABI exists;
- broad vendor interoperability matrix beyond the real servers explicitly tested by the helper scripts.

Recommended release validation before promoting beyond technical preview:

```sh
cmake/qt-viewer-smoke.sh /tmp/uvnc-qt-viewer-build /tmp/uvnc-qt-viewer-install
cmake/qt-viewer-rfb-smoke.sh /tmp/uvnc-qt-viewer-rfb-build /tmp/uvnc-qt-viewer-rfb-install
cmake/qt-viewer-real-server-matrix-smoke.sh 127.0.0.1 5901 /tmp/uvnc-real-server-build /tmp/uvnc-real-server-install <VNC_PASSWORD>
UVNC_VIEWER_REAL_SERVER_ENCODINGS=hextile,zlib,zrle,tight,raw,copyrect,newfbsize cmake/qt-viewer-real-server-matrix-smoke.sh 127.0.0.1 5901 /tmp/uvnc-real-server-compressed-build /tmp/uvnc-real-server-compressed-install <VNC_PASSWORD>
cmake/qt-viewer-long-real-server-smoke.sh 127.0.0.1 5901 /tmp/uvnc-long-real-server-build /tmp/uvnc-long-real-server-install <VNC_PASSWORD> 20
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

### CLI operational viewer file-transfer

The native Linux viewer exposes file-transfer operations through CLI flags for automation and smoke tests:

```bash
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --list-remote /
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --list-drives
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --download-remote remote.txt --download-output /tmp/remote.txt
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --upload-local /tmp/local.txt --upload-remote uploaded.txt
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --remote-checksums remote.txt
```

### Sanitized viewer config summary

Use `--print-config` to inspect the effective viewer configuration without printing the password value:

```bash
uvnc_qt_viewer --host 127.0.0.1 --port 5901 --password secret --print-config
```

### VeNCrypt/TLS portable viewer smoke

The portable viewer path includes a local VeNCrypt/X509Vnc smoke test against the native memory server. It verifies the VeNCrypt negotiation, TLS transport, VNCAuth over TLS and framebuffer update path:

```bash
cmake --build /tmp/uvnc-viewer-tls-build --target vncviewer_portable_vencrypt_tls_session_smoke
ctest --test-dir /tmp/uvnc-viewer-tls-build --output-on-failure -R vncviewer_portable_vencrypt_tls_session_smoke
```
