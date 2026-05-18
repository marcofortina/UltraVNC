# Native Linux PipeWire/XDG portal capture skeleton

The PipeWire/XDG portal backend is intentionally introduced as an optional
skeleton before full frame import is wired into the portable framebuffer path.
It is selected at runtime with:

```sh
uvnc_winvnc_memory_server --capture-backend pipewire
```

The backend is built only when the optional development files are available and
`ULTRAVNC_BUILD_LINUX_CAPTURE_PIPEWIRE=ON` is enabled.

## Requirements

- A Wayland session or `WAYLAND_DISPLAY`.
- A D-Bus session bus.
- XDG desktop portal service available for the desktop environment.
- PipeWire development files for build-time detection.

On Debian/Ubuntu development systems, the expected packages are:

```sh
sudo apt install libpipewire-0.3-dev libdbus-1-dev xdg-desktop-portal
```

## CI behavior

The helper below builds the optional backend and exercises the runtime-detection
path. It does not fail CI when a real PipeWire/XDG portal session is unavailable:

```sh
cmake/pipewire-capture-smoke.sh \
  /tmp/uvnc-pipewire-capture-build \
  /tmp/uvnc-pipewire-capture-install
```

## Manual validation

Run this from a real Wayland desktop session, not from a plain SSH shell:

```sh
echo "$XDG_SESSION_TYPE"
echo "$WAYLAND_DISPLAY"
echo "$DBUS_SESSION_BUS_ADDRESS"

cmake/pipewire-capture-smoke.sh \
  /tmp/uvnc-pipewire-capture-build \
  /tmp/uvnc-pipewire-capture-install

/tmp/uvnc-pipewire-capture-install/bin/uvnc_winvnc_memory_server \
  --smoke-pipewire-availability-test
```

## Current limitation

This milestone provides build-time detection, runtime detection, portal session
scaffolding and stream descriptor scaffolding. It does not yet import live
PipeWire buffers into the portable framebuffer path.
