# Native Linux porting status

This document tracks the experimental native Linux support work for UltraVNC.
It is intentionally conservative: it lists what is currently buildable on Linux
and what remains Windows-only.

## Current native Linux subset

The current CMake subset builds and tests the following components natively on Linux:

- `librdr` stream/protocol support.
- MiniLZO compression support.
- Classic VNC auth/des helper library used by portable tests.
- Experimental `uvnc_repeater_headless` executable.
- A growing WinVNC portable core slice:
  - portable region/update tracking;
  - translation tables;
  - compression helpers;
  - raw/RRE/hextile/CoRRE/zlib/zlibhex/ultra/ultra2/XZ/tight/ZRLE encoder paths;
  - portable runtime hooks for monotonic time, sleeping, process id, environment lookup
    and shutdown signalling;
  - portable framebuffer, dirty tracking, desktop-source, capture-pipeline and
    update-encoder abstractions used as the first Linux server-side seam;
  - portable TCP, RFB handshake/session/message parsing and raw framebuffer
    update serving for the experimental memory server;
  - configurable in-memory framebuffer fill byte, synthetic framebuffer patterns
    and multi-update smoke paths.

Run the local validation helper from the repository root:

```sh
cmake/native-linux-subset-smoke.sh /tmp/uvnc-linux-build /tmp/uvnc-linux-install
```

For faster WinVNC-only iterations:

```sh
cmake/winvnc-portable-core-smoke.sh /tmp/uvnc-winvnc-portable-core-build
```


## Closed native Linux milestones

The following milestones are considered closed for this incremental Linux-support
track:

- **Repeater Linux**: the headless Linux repeater target builds, validates,
  smokes loopback listeners and installs through the dedicated closure helper.
- **WinVNC portable core**: the native Linux portable WinVNC core builds and
  exercises protocol/update/encoder/runtime/framebuffer/RFB session seams through
  the dedicated closure helper.
- **Memory server RFB**: the experimental native Linux memory server exercises
  deterministic RFB handshake, client-message processing, raw framebuffer
  updates, multi-update sessions and synthetic framebuffer patterns through the
  dedicated closure helper.

Use `docs/native-linux-milestone-closure.md` for the exact closure scope and
validation commands.

## Still Windows-only

The following areas are not native Linux implementations yet:

- Real Linux desktop capture and input injection backends.
- Full Linux daemon/service/session integration around the portable runtime hooks.
- Native viewer UI and Windows-specific message loop integration.
- MFC/Win32 dialogs, resources, registry integration and tray UI.
- Windows socket/window compatibility layers outside the isolated portable subset.

## Porting rule

Do not pretend that Win32 UI/service/capture code can simply compile on Linux.
Move protocol, encoding and runtime-neutral code into tested portable slices first,
keep Linux runtime/capture seams explicit, then add Linux-specific implementations
for desktop capture, input, daemon/service integration and UI paths.

Native Linux X11 capture progress:

- X11 capture backend selection is wired into the memory server.
- The baseline X11 path uses XGetImage when a usable DISPLAY is available.
- Optional XShm support is detected and used when built and available.
- Headless CI keeps the X11 live smoke non-fatal when DISPLAY is absent.

Native Linux PipeWire/XDG portal capture progress:

- PipeWire capture backend selection is wired into the registry and CLI.
- Build-time detection is optional and does not make the Linux subset depend on PipeWire.
- Runtime detection checks Wayland and D-Bus session prerequisites.
- Portal session and PipeWire stream scaffolding are present for the next implementation step.
- Headless CI keeps PipeWire live capture non-fatal when the portal runtime is unavailable.

Native Linux input backend progress:

- Input backend registry and runtime selection are available.
- X11/XTest build/runtime availability is detected when XTest development files are installed.
- XTest keyboard and pointer injection paths are present behind explicit backend selection.
- Automated smoke coverage remains non-invasive; live input injection requires explicit opt-in.
- Wayland/PipeWire input injection remains a documented limitation, not an implemented backend.

Native Linux server backend integration progress:

- Example user-session service and environment templates are generated and installed.
- Capture and input backend configuration is documented for the current Linux server path.
- Manual validation steps are documented for X11 capture, PipeWire/XDG portal scaffolding and XTest input.
- The integration smoke verifies installed templates, installed docs and backend CLI wiring.


Native Linux CI gate progress:

- The aggregate `cmake/native-linux-ci-gate-smoke.sh` helper is the local source
  of truth for CI-safe native Linux validation.
- The GitHub Actions `Native Linux subset` workflow runs that aggregate helper
  instead of hand-maintaining a long list of duplicate smoke steps.
- The gate is intentionally headless/CI-safe and documents live checks that must
  still be run manually.
- PR creation and GitHub check inspection are deferred until the final review
  stage; do not claim GitHub CI is green until a real workflow run has been
  inspected.


Native Linux Qt viewer progress:

- Qt is the selected frontend toolkit for the native Linux viewer path.
- The experimental `uvnc_qt_viewer` target builds independently of the Windows viewer UI.
- The Qt shell now has a framebuffer surface widget with deterministic offscreen smoke coverage.
- Keyboard and pointer events are captured inside the Qt surface for future RFB forwarding.
- `uvnc_qt_viewer --connect-update-smoke` connects to the native memory server and reads one raw framebuffer update.
- Long-running remote session ownership and live Qt surface updates are still follow-up work.
