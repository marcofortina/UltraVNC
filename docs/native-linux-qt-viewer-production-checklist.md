# Native Linux Qt viewer production-readiness checklist

The native Linux Qt viewer is now a real executable that can connect to VNCAuth-capable VNC servers and render framebuffer updates through the Qt surface.

Production-ready scope currently covered:

- interactive host/port/password/options UI;
- profile load/save through Qt settings;
- reconnect/disconnect/update controls;
- optional auto reconnect;
- VNCAuth security type 2 challenge/response;
- raw, CopyRect, RRE, CoRRE, Hextile, Zlib, ZRLE, Tight and NewFBSize negotiation/handling; ZRLE covers raw, solid, packed-palette and RLE tiles;
- multi-rectangle FramebufferUpdate handling;
- ClientCutText clipboard send path;
- ServerCutText receive display;
- Bell tolerance;
- keyboard/pointer forwarding path;
- desktop entry/icon/docs install artifacts;
- loopback, portable and real-server smoke helpers.

Known limits still outside the current production baseline:

- compressed encoding decoders not yet implemented for Tight JPEG/gradient, ZlibHex and vendor-specific extensions;
- extended clipboard protocol;
- encrypted transport/TLS security types;
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
