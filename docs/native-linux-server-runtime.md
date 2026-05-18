# Native Linux server runtime notes

This document tracks the current native Linux server runtime boundary for the
incremental #348 work.

The runtime is still experimental, but it is no longer limited to synthetic
harness-only paths:

- `capture_backend=x11` captures the active X11 root window through the portable
  framebuffer path and refreshes the framebuffer before served update requests;
- `input_backend=xtest` can route RFB key and pointer events to the active X11
  session when XTest is built and available;
- `capture_backend=memory` and `capture_backend=raw-file` remain deterministic
  CI/lab backends;
- PipeWire/XDG portal remains runtime-detected scaffolding, not live frame import.

## Installed files

A CMake install places Linux runtime examples under:

```sh
${prefix}/share/ultravnc/linux/
```

Relevant files:

```text
uvnc-winvnc-memory-server.service
uvnc-winvnc-memory-server.env.example
uvnc-winvnc-linux-server.conf.example
native-linux-server-backend-integration.md
native-linux-input-backend.md
native-linux-pipewire-capture.md
native-linux-server-runtime.md
```

## Runtime config file

Copy the example config into the user session config directory:

```sh
mkdir -p ~/.config/ultravnc
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example \
  ~/.config/ultravnc/uvnc-winvnc-linux-server.conf
```

Conservative starting point:

```ini
bind_address=127.0.0.1
port=5901
name=uvnc-linux-server
capture_backend=auto
input_backend=none
max_updates=1024
serve_updates=true
```

Use `capture_backend=x11` only from a real local X11 session. Keep
`input_backend=none` until live input injection has been explicitly validated.

Validate and inspect the resolved runtime config:

```sh
uvnc_winvnc_memory_server \
  --config ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  --validate-config

uvnc_winvnc_memory_server \
  --config ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  --print-config
```

## Manual X11 validation

Run from the local graphical X11 session, not from SSH X forwarding:

```sh
echo "$DISPLAY"
echo "${XDG_SESSION_TYPE:-unset}"

uvnc_winvnc_memory_server --smoke-x11-availability-test
uvnc_winvnc_memory_server --validate-config --capture-backend x11
uvnc_winvnc_memory_server --smoke-x11-update-test --name x11-live-capture-smoke
```

Expected local-session shape:

```text
XDG_SESSION_TYPE=x11
DISPLAY=:0
```

`DISPLAY=localhost:10.0`, `127.0.0.1:*`, or missing `DISPLAY` are not valid
local desktop capture targets for this milestone.

## Manual XTest validation

Live XTest validation intentionally requires explicit opt-in because it injects
input into the active X11 session:

```sh
uvnc_winvnc_memory_server --smoke-xtest-availability-test
UVNC_RUN_XTEST_LIVE=1 cmake/linux-input-smoke.sh \
  /tmp/uvnc-linux-input-build \
  /tmp/uvnc-linux-input-install
```

For the service config, switch to:

```ini
input_backend=xtest
```

only after the availability smoke and live input smoke pass in the target user
session.

## systemd user service

The installed service is a user-session service. That is deliberate: X11 capture
and XTest input belong to a graphical user session and should not pretend to be a
system daemon that can capture arbitrary desktops.

Example setup:

```sh
mkdir -p ~/.config/ultravnc
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example \
  ~/.config/ultravnc/uvnc-winvnc-linux-server.conf
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.env.example \
  ~/.config/ultravnc/uvnc-winvnc-memory-server.env

systemctl --user link /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.service
systemctl --user daemon-reload
systemctl --user start uvnc-winvnc-memory-server.service
systemctl --user status uvnc-winvnc-memory-server.service
```

The runtime writes a pid file and a coarse status file while running. The unit
uses the systemd user runtime directory for those files.

## Failure modes

Prefer clear failure over silent fallback for explicitly selected backends:

- `--capture-backend x11` fails when no usable X11 display is available;
- `--input-backend xtest` fails when XTest is not built or not available on the
  active X11 display;
- `--capture-backend pipewire` remains unavailable until live frame import is
  implemented;
- `--capture-backend auto` may fall back to `memory`, so use explicit `x11` for
  real desktop validation.

## Current limits

- The current RFB security path is still no-auth and must remain loopback/lab
  scoped until the security/config hardening milestone is completed.
- Live X11 capture assumes stable framebuffer geometry during a session; live
  resize/NewFBSize handling belongs to a follow-up server milestone.
- Wayland input injection is intentionally not implemented here.
