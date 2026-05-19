# Native Linux service/admin equivalents

This document maps the Windows-only WinVNC service, tray and settings workflows to the native Linux server milestone.

## Decision

The native Linux server does **not** port the Windows service manager, tray UI or settings UI directly. Those workflows are platform-specific Windows code and depend on Windows service control, desktop switching and interactive tray behavior.

The Linux equivalents are:

| Windows concept | Native Linux equivalent |
| --- | --- |
| Windows service install/start/stop | systemd user service `uvnc-winvnc-memory-server.service` |
| Service Control Manager status | `systemctl --user status uvnc-winvnc-memory-server.service` |
| Tray status icon | `%t/uvnc-winvnc-memory-server.status`, journal logs, CLI summary |
| Settings UI | `~/.config/ultravnc/uvnc-winvnc-linux-server.conf` plus `--validate-config` |
| Admin diagnostics | `--print-config`, `--print-admin-summary`, status/log/pid files |
| Windows-only DSM/MSLogon/plugin UI | unsupported/fail-closed policy on Linux |
| HTTP Java viewer toggle | legacy disabled policy on Linux |

## Operator lifecycle

Install the unit and examples, then copy the examples into the user configuration directory:

```sh
mkdir -p ~/.config/ultravnc
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-linux-server.conf.example \
  ~/.config/ultravnc/uvnc-winvnc-linux-server.conf
cp /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.env.example \
  ~/.config/ultravnc/uvnc-winvnc-memory-server.env
chmod 600 ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  ~/.config/ultravnc/uvnc-winvnc-memory-server.env
```

Validate before starting:

```sh
uvnc_winvnc_memory_server \
  --config ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  --validate-config
```

Review the Linux admin summary:

```sh
uvnc_winvnc_memory_server \
  --config ~/.config/ultravnc/uvnc-winvnc-linux-server.conf \
  --print-admin-summary
```

Start/stop through systemd user units:

```sh
systemctl --user link /usr/local/share/ultravnc/linux/uvnc-winvnc-memory-server.service
systemctl --user daemon-reload
systemctl --user start uvnc-winvnc-memory-server.service
systemctl --user status uvnc-winvnc-memory-server.service
systemctl --user stop uvnc-winvnc-memory-server.service
```

Runtime files are written under the user runtime directory:

```text
$XDG_RUNTIME_DIR/uvnc-winvnc-memory-server.pid
$XDG_RUNTIME_DIR/uvnc-winvnc-memory-server.status
$XDG_RUNTIME_DIR/uvnc-winvnc-memory-server.log
```

## Explicit non-goals for this milestone

- No Linux tray icon is shipped.
- No Linux GUI settings editor is shipped.
- No Windows service-control compatibility layer is shipped.
- No DSM/MSLogon/security-plugin UI is ported.
- No HTTP Java viewer service is enabled.

Those are not silently missing features: they are documented platform boundaries. Future Linux admin UI work should be a native Qt/admin tool or a separate management frontend, not a direct port of Windows tray/service UI code.
