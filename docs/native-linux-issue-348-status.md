# Issue #348 native Linux status

This document summarizes the current native Linux support status for issue #348.
It is intended to keep the GitHub checklist aligned with the implemented Linux
code, local gates and remaining validation work.

## Main roadmap

Implemented locally:

- dedicated Linux support branch workflow;
- guarded native Linux CMake path;
- Windows-only applications disabled on non-Windows targets;
- portable protocol/runtime libraries built natively;
- headless repeater, memory server, native Linux server, Qt viewer and Qt server settings targets;
- Linux capture/input/runtime abstractions;
- CI/local smoke helpers for the Linux subset.

Still requiring repository/PR evidence:

- first coherent PR for completed Linux milestones;
- GitHub CI verification on that PR;
- required checks green on GitHub;
- follow-up PR slicing for server/viewer/UI milestones if the full branch is too large.

## Native Linux server and admin

Implemented locally:

- raw-file, memory and X11 capture paths;
- PipeWire/XDG portal scaffolding and documented limitations;
- XTest input backend;
- user-mode and systemd service runtime controls;
- config validation, admin summary, status/log/pid files;
- Qt server settings/admin GUI with live start/stop/status/log;
- legacy executable names: `winvnc`, `uvnc_settings`, `setpasswd`, `createpassword`, `repeater`;
- MSLogonII server auth through external helper;
- DSM provider ABI and native SecureVNC provider artifact.

Remaining server-side validation:

- real desktop/operator validation artifacts for X11/XTest on the target host;
- visual review artifact for Qt settings UI parity;
- optional future Wine/IPC bridge only if Windows `.dsm` binary loading is explicitly required.

## Native Linux viewer

Implemented locally:

- Qt viewer executable and legacy `vncviewer` alias;
- interactive connection UI, profiles/config file support and install artifacts;
- persistent RFB sessions, input, clipboard, file transfer helpers and TLS/VeNCrypt;
- Raw, CopyRect, RRE, CoRRE, Hextile, Zlib, ZRLE, Tight baseline/JPEG/gradient, ZlibHex and NewFBSize coverage;
- MSLogonII viewer auth support;
- local Qt viewer smoke helpers and production gate scripts.

Remaining viewer validation:

- external real-server matrix with x11vnc;
- external real-server matrix with LibVNCServer;
- external real-server matrix with UltraVNC server;
- external real-server matrix with RealVNC;
- required multi-vendor matrix gate green with those real servers.

## Cross-platform Qt UI convergence

Implemented locally:

- minimal and functional Qt viewer shell;
- minimal and functional Qt server/admin/settings shell;
- shared viewer behavior moved toward portable viewer core + Qt UI;
- shared server/admin/settings behavior moved into Qt settings/admin UI and portable server config model;
- platform-specific behavior isolated behind Linux runtime/service/provider boundaries.

Remaining:

- human visual review before claiming pixel-perfect Win32 parity;
- decision on whether Windows UI replacement should happen later or remain side-by-side.

## Recommended checkbox updates

The GitHub issue checklist should now mark the following previously-open items as complete when the corresponding local logs are attached:

- Add a minimal Qt server/admin/settings shell target.
- Move shared server/admin/settings behavior toward the Qt UI layer.
- Keep platform-specific integrations isolated under platform backends.

Keep these unchecked until external evidence exists:

- Verify the CI workflow on an opened PR.
- Confirm required checks are green on GitHub.
- Prepare the first coherent PR for the completed Linux foundation milestones.
- Prepare follow-up PRs for server backend work instead of mixing them into the foundation PR.
- Real-server viewer matrix entries for x11vnc, LibVNCServer, UltraVNC server and RealVNC.
- Run the required multi-vendor matrix gate successfully.
