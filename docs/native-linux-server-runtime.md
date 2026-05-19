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

Conservative installed starting point:

```ini
bind_address=127.0.0.1
port=5901
name=uvnc-linux-server
capture_backend=auto
input_backend=none
max_updates=1024
serve_updates=true
# auth=vnc-password
# password_file=/home/USER/.config/ultravnc/vnc-password
```

The installed example is intentionally fail-closed: it does not enable no-auth
and it requires the operator to configure a private password file before real
use. No-auth is disabled by default in the binary. `allow_no_auth=true` is an
explicit lab opt-in and is acceptable only for loopback validation. Use
`capture_backend=x11` only from a real local X11 session. Keep
`input_backend=none` until live input injection has been explicitly validated.


## Security and authentication policy

The experimental Linux server now fails closed for unsafe no-auth runtime paths:

- `auth=none` requires `allow_no_auth=true`;
- `auth=none` on a non-loopback bind additionally requires
  `allow_public_no_auth=true`, which is intended only for controlled lab tests;
- `password_file` must point to a regular private file that is not accessible by
  group or other users;
- VNCAuth passwords are limited to 8 bytes by the RFB legacy VNCAuth design;
- VNCAuth on non-loopback bind addresses is blocked by default because this
  milestone does not implement transport encryption; use
  `allow_unencrypted_public=true` only for explicit controlled exposure behind
  firewall, tunnel or trusted-network policy;
- `--print-config` reports the selected auth mode but never prints the password.

Recommended private password-file setup:

```sh
mkdir -p ~/.config/ultravnc
install -m 0600 /dev/null ~/.config/ultravnc/vnc-password
printf '%s\n' 'secret1' > ~/.config/ultravnc/vnc-password
```

Then configure:

```ini
auth=vnc-password
password_file=/home/USER/.config/ultravnc/vnc-password
```

For any LAN bind, VNCAuth is the minimum accepted auth mode, but this milestone
still refuses non-loopback VNCAuth unless `allow_unencrypted_public=true` is set.
That opt-in is intentionally noisy: VNCAuth authenticates the handshake, but it
does not encrypt framebuffer, clipboard or input traffic. Keep the service behind
trusted-network, firewall or tunnel controls until a real transport security type
lands.

## Bind-address policy

Recommended stages:

```ini
# local production-like baseline with legacy VNCAuth
bind_address=127.0.0.1
auth=vnc-password
password_file=/home/USER/.config/ultravnc/vnc-password
```

```ini
# local smoke/lab only, not production
bind_address=127.0.0.1
auth=none
allow_no_auth=true
```

```ini
# LAN/lab with legacy VNCAuth
bind_address=192.0.2.10
auth=vnc-password
password_file=/home/USER/.config/ultravnc/vnc-password
allow_unencrypted_public=true
```

Avoid `bind_address=0.0.0.0` unless the host firewall and network exposure are
understood. The binary prints a warning for all-interface binds because transport
TLS is not implemented in this milestone.

## TLS/security-type strategy

The native Linux server now supports a real TLS transport security path using
VeNCrypt with the X509Vnc subtype. This is not DSM plugin emulation and it does
not pretend that legacy VNCAuth encrypts traffic. The production baseline is:

1. `transport_security=vencrypt-x509-vnc`;
2. `auth=vnc-password` with a private `password_file`;
3. absolute `tls_certificate_file` and `tls_private_key_file` paths;
4. TLS private key permissions not accessible by group/other;
5. non-loopback unencrypted VNCAuth still refused unless explicitly overridden.

Example:

```ini
auth=vnc-password
password_file=/home/USER/.config/ultravnc/vnc-password
transport_security=vencrypt-x509-vnc
tls_certificate_file=/home/USER/.config/ultravnc/server.crt
tls_private_key_file=/home/USER/.config/ultravnc/server.key
```

For self-signed lab certificates, generate the key locally and keep it private:

```bash
install -m 0700 -d ~/.config/ultravnc
openssl req -x509 -newkey rsa:3072 -nodes \
  -keyout ~/.config/ultravnc/server.key \
  -out ~/.config/ultravnc/server.crt \
  -subj "/CN=$(hostname -f)" \
  -days 397
chmod 0600 ~/.config/ultravnc/server.key
chmod 0644 ~/.config/ultravnc/server.crt
```

Client trust is still an operator responsibility: use a client that implements
VeNCrypt/X509Vnc and validate or pin the server certificate according to your
environment. DSM/security plugins and MSLogon remain Windows-specific legacy
features and are not loaded by the native Linux server.

## Secure runtime validation helper

Use this helper for the production-like local gate. It creates a private
VNCAuth password file, verifies that group-readable password files are rejected,
checks that `--print-config` does not expose password material, and exercises
RFB VNCAuth handshake/update paths without enabling no-auth:

```sh
cmake/linux-server-secure-runtime-smoke.sh \
  /tmp/uvnc-linux-server-secure-runtime-build \
  /tmp/uvnc-linux-server-secure-runtime-install
```

To combine VNCAuth with real X11 capture, run from a local X11 graphical
session and opt in explicitly:

```sh
UVNC_RUN_SECURE_X11_SERVER=1 cmake/linux-server-secure-runtime-smoke.sh \
  /tmp/uvnc-linux-server-secure-runtime-build \
  /tmp/uvnc-linux-server-secure-runtime-install
```

This is the preferred validation path for production-like server runs in this
milestone. The older no-auth live runtime smoke remains loopback/lab-only.

## Real runtime validation helper

Use the repository helper below for the next server milestone validation. By
default it is CI-safe and stops after build/install/config/availability checks.

```sh
cmake/linux-server-real-runtime-smoke.sh \
  /tmp/uvnc-linux-server-real-runtime-build \
  /tmp/uvnc-linux-server-real-runtime-install
```

To run the real X11 server runtime path, execute it from the local graphical X11
session and opt in explicitly:

```sh
UVNC_RUN_REAL_X11_SERVER=1 cmake/linux-server-real-runtime-smoke.sh \
  /tmp/uvnc-linux-server-real-runtime-build \
  /tmp/uvnc-linux-server-real-runtime-install
```

The opt-in path starts `uvnc_winvnc_memory_server` with `capture_backend=x11`,
`input_backend=none`, an ephemeral loopback port, pid/status/log files, and
`--serve-forever`. It then connects as a minimal RFB client, requests one raw
framebuffer update, sends SIGTERM, and verifies that the pid file is removed and
the status reaches `stopped`.

This is the first real runtime validation gate for X11 capture, server process
state and graceful shutdown. It still does not replace the later multi-vendor
viewer compatibility matrix.

Negative runtime cases are covered by:

```sh
cmake/linux-server-negative-runtime-smoke.sh \
  /tmp/uvnc-linux-server-negative-runtime-build \
  /tmp/uvnc-linux-server-negative-runtime-install
```

That helper checks invalid config, invalid backend names, unwritable log paths,
occupied TCP ports, and explicit X11/XTest backend failures with `DISPLAY`
unset.

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
uvnc_winvnc_memory_server \
  --config ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  --print-admin-summary
```

`--print-admin-summary` is the native Linux replacement for the Windows tray/settings/service overview. It reports the selected Linux service, config/status/log mechanism and explicitly disabled legacy Windows-only features.

The runtime writes pid, status and log files while running. The unit passes
these paths explicitly with the systemd user runtime directory, so the example
does not rely on environment-file specifier expansion for runtime state paths.

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

- VNCAuth is available, but does not encrypt framebuffer/input/clipboard traffic unless paired with VeNCrypt/X509Vnc.
- Live X11 capture supports resize/NewFBSize and dirty-region refinement, but real viewer matrix validation remains a final gate.
- Wayland input injection is intentionally not implemented here.
