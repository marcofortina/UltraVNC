# Native Linux legacy parity status

This document tracks parity between the original Windows-only UltraVNC deliverables and the native Linux foundation.

## Executable names

Native Linux builds now expose the same user-facing executable names as the legacy Windows deliverables, without the `.exe` suffix:

| Legacy Windows executable | Native Linux executable | Native Linux implementation |
| --- | --- | --- |
| `winvnc.exe` | `winvnc` | alias for `uvnc_winvnc_memory_server` |
| `vncviewer.exe` | `vncviewer` | alias for `uvnc_qt_viewer` |
| `uvnc_settings.exe` | `uvnc_settings` | alias for `uvnc_qt_server_settings` |
| `repeater.exe` | `repeater` | alias for `uvnc_repeater_headless` |
| `setpasswd.exe` | `setpasswd` | alias for `uvnc_winvnc_password_file` |
| `SecureVNCPlugin.dsm` | `SecureVNCPlugin.dsm` | native Linux DSM-provider shared object |

The longer `uvnc_*` names remain internal/compatibility targets so existing tests and scripts do not break.

## Windows DSM plugins

Original Windows `.dsm` plugins are PE/COFF DLLs. Native Linux cannot safely `dlopen()` those plugins directly.  The native Linux DSM loader detects PE/COFF DSM files and fails with a clear diagnostic instead of a loader crash or misleading `dlopen()` error.

Supported provider formats:

- native Linux DSM provider ABI loaded from `.so` or `.dsm` ELF shared objects;
- native `SecureVNCPlugin.dsm` provider built by this tree.

Unsupported without a separate compatibility layer:

- loading the original Windows `SecureVNCPlugin.dsm` PE/COFF binary directly inside the native Linux process.

A future Wine/IPC bridge can be added as a separate provider if that exact binary compatibility is required.

## Visual parity review

The Qt server settings UI exposes reproducible parity evidence:

```sh
uvnc_settings --print-visual-parity-report
uvnc_settings --save-visual-parity-snapshot /tmp/uvnc-settings-parity.png
```

The report lists the WinVNC-compatible sections and key runtime/security fields. The snapshot is intended for real host visual review against the original WinVNC dialogs.

Pixel-perfect parity still requires a human visual review on a Qt host with the desired desktop theme, because the original Win32 dialogs and Linux Qt widgets use different native style engines.
