Summary
-------
Continue #348 as a real native Linux support milestone branch.

This PR adds the native Linux foundation, headless repeater, portable WinVNC/RFB core, native Linux server runtime/backend work, Qt viewer, Qt server settings/admin tooling, legacy executable parity and Linux-native security provider scaffolding.

Changes
-------
- guarded native Linux CMake path with Windows-only targets disabled on non-Windows;
- native Linux headless repeater;
- portable WinVNC/RFB core and memory server coverage;
- native Linux server capture/input/runtime backends and systemd/user-mode support;
- Qt viewer and Qt server settings/admin tools;
- legacy executable aliases without `.exe`: `winvnc`, `vncviewer`, `uvnc_settings`, `repeater`, `setpasswd`, `createpassword`;
- MSLogonII viewer/server support through Linux external auth helper;
- native Linux DSM provider ABI and SecureVNC provider artifact;
- local Linux parity, issue-status and viewer/server smoke gates.

Validation
----------
- `cmake/native-linux-core-parity-smoke.sh ...`
- `cmake/native-linux-legacy-parity-smoke.sh ...`
- `cmake/native-linux-issue-348-status-smoke.sh`

Notes
-----
- Windows PE/COFF `.dsm` plugins are not loaded directly on Linux; native Linux providers are supported through the new ABI.
- Multi-vendor external matrix evidence remains follow-up unless attached to this PR.
- Human visual review is still required before claiming pixel-perfect Win32 UI parity.

Refs #348
