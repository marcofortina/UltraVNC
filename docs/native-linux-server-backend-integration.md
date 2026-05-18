# Native Linux server backend integration and validation

This document describes the current integration boundary for the experimental
native Linux server path.

The native Linux server work is still incremental. The available binary is
`uvnc_winvnc_memory_server`, which exercises the portable WinVNC/RFB server core
with selectable Linux capture and input backends. The X11 path now serves live
source snapshots through the portable framebuffer/update path, but this is still
not a production replacement for the Windows WinVNC service.

## Build and smoke

```sh
cmake/linux-server-integration-smoke.sh \
  /tmp/uvnc-linux-server-integration-build \
  /tmp/uvnc-linux-server-integration-install
```

The helper validates:

- integration templates are generated and installable;
- capture backend selection is wired through `--capture-backend`;
- input backend selection is wired through `--input-backend`;
- X11/PipeWire/input smoke helpers remain CI-safe when the runtime is missing;
- installed `uvnc_winvnc_memory_server` can validate/print selected backend config;
- installed config, env and systemd user-service examples are present.

## Runtime backend selection

Capture backend:

```sh
uvnc_winvnc_memory_server --capture-backend auto
uvnc_winvnc_memory_server --capture-backend memory
uvnc_winvnc_memory_server --capture-backend raw-file --raw-framebuffer-file /path/to/framebuffer.raw
uvnc_winvnc_memory_server --capture-backend x11
uvnc_winvnc_memory_server --capture-backend pipewire
```

Input backend:

```sh
uvnc_winvnc_memory_server --input-backend auto
uvnc_winvnc_memory_server --input-backend none
uvnc_winvnc_memory_server --input-backend xtest
```

The safe default for service templates is `input_backend=none`. Live input
injection must remain opt-in and should only be enabled after XTest live smoke
validation passes in the target user session.

## Systemd user-service template

The build installs example integration files under:

```sh
${prefix}/share/ultravnc/linux/
```

The generated service file is a template for user-session deployment, not a
distro package policy decision. X11/PipeWire capture and XTest input operate in a
user graphical session, so a system user service is safer than pretending that a
system daemon can capture arbitrary desktops without session-specific setup.

Example setup after installation:

```sh
mkdir -p ~/.config/ultravnc
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example \
  ~/.config/ultravnc/uvnc-winvnc-linux-server.conf
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.env.example \
  ~/.config/ultravnc/uvnc-winvnc-memory-server.env

systemctl --user link /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.service
systemctl --user daemon-reload
systemctl --user start uvnc-winvnc-memory-server.service
```

Adjust paths if a different install prefix is used.

## Manual X11 validation

Run from a real local X11 session, not from SSH X forwarding:

```sh
echo "$DISPLAY"
echo "$XDG_SESSION_TYPE"
xdpyinfo >/dev/null

cmake/x11-capture-smoke.sh \
  /tmp/uvnc-x11-capture-build \
  /tmp/uvnc-x11-capture-install
```

Expected local-session properties:

```text
XDG_SESSION_TYPE=x11
DISPLAY=:0
```

`DISPLAY=localhost:10.0` means SSH X forwarding and is intentionally skipped.

## Manual PipeWire/XDG portal validation

Run from a real Wayland desktop session with a session D-Bus bus and portal
service:

```sh
echo "$XDG_SESSION_TYPE"
echo "$WAYLAND_DISPLAY"
echo "$DBUS_SESSION_BUS_ADDRESS"

cmake/pipewire-capture-smoke.sh \
  /tmp/uvnc-pipewire-capture-build \
  /tmp/uvnc-pipewire-capture-install
```

This milestone validates scaffolding and runtime detection. Live PipeWire frame
import remains a later capture-backend milestone.

## Manual XTest validation

Run only from a local X11 session where moving/injecting input is acceptable:

```sh
UVNC_RUN_XTEST_LIVE=1 cmake/linux-input-smoke.sh \
  /tmp/uvnc-linux-input-build \
  /tmp/uvnc-linux-input-install
```

Live injection requires explicit opt-in and must not be enabled by default in CI.

## Runtime operator notes

See `native-linux-server-runtime.md` for the current config file format,
systemd user-service flow, status/pid files, X11 validation, XTest validation,
failure modes and current production limitations.
