# Native Linux input backend notes

This document tracks the first native Linux input-injection milestone for the
incremental #348 Linux support work.

## Backends

- `none`: disables input injection and is always available.
- `xtest`: uses the X11 XTest extension when it is built and available on the
  active X11 display.
- `auto`: selects `xtest` when available, otherwise falls back to `none`.

When `input_backend=xtest` is selected in the native Linux server runtime, RFB
key and pointer events are routed through the XTest backend instead of being only
recorded in the portable session state. Explicit `xtest` selection fails when
XTest is unavailable; use `none` when input injection must be disabled.

Wayland/PipeWire input injection is not implemented in this milestone. It needs
a separate design because Wayland compositors intentionally restrict synthetic
input and the correct path depends on compositor/portal policy.

## Build dependencies

On Debian/Ubuntu systems, the XTest backend needs:

```sh
sudo apt install -y libxtst-dev libxi-dev libx11-dev
```

The build remains optional: if the development files are absent, the backend is
compiled out and reports itself as unavailable.

## Non-invasive smoke

The default smoke validates selection and availability without injecting input:

```sh
cmake/linux-input-smoke.sh /tmp/uvnc-linux-input-build /tmp/uvnc-linux-input-install
```

## Live X11/XTest validation

Live input injection is opt-in because it affects the active X11 session:

```sh
UVNC_RUN_XTEST_LIVE=1 cmake/linux-input-smoke.sh /tmp/uvnc-linux-input-build /tmp/uvnc-linux-input-install
```

The helper skips live injection when `DISPLAY` is missing, when the session is
not X11, or when `DISPLAY` looks like SSH X forwarding.

## Runtime failure modes

- Missing `DISPLAY`: explicit `xtest` fails; `auto` falls back to `none`.
- Wayland session: explicit `xtest` fails unless an X11/XTest display is actually
  available. Wayland-native injection is intentionally out of scope here.
- SSH X forwarding: live injection helpers skip it because it is not a safe local
  desktop validation target.
