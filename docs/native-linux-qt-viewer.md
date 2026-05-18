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

## Current scope

Implemented in this milestone:

- Qt6 Widgets shell target: `uvnc_qt_viewer`.
- Portable viewer configuration parsing.
- CLI validation and smoke mode.
- Offscreen Qt event-loop smoke.
- Qt framebuffer surface widget with deterministic synthetic pixels.
- Local keyboard and pointer event handling inside the Qt surface.

Not implemented in this milestone:

- RFB network session ownership by the Qt viewer.
- RFB-backed framebuffer updates from a remote server.
- Keyboard/pointer forwarding to a remote server.
- Clipboard and file transfer.
