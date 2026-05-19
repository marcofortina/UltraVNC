# Native Linux Qt server settings

`uvnc_qt_server_settings` is the native Linux counterpart for the Windows server/settings configuration workflow.  It does not try to load the Win32 `uvnc_settings` dialogs; it drives the portable Linux server configuration model directly and writes config files consumed by `uvnc_winvnc_memory_server`.

Covered settings:

- bind address and port;
- framebuffer size and desktop name;
- auth mode: no-auth lab mode, VNCAuth, MSLogonII external helper;
- no-auth/public-bind guard flags;
- VeNCrypt/X509Vnc TLS certificate and private-key paths;
- file-transfer mode/root/overwrite policy;
- shared-client limit;
- update pacing;
- extended clipboard and initial server clipboard text.

Examples:

```sh
uvnc_qt_server_settings --smoke-test
uvnc_qt_server_settings --print-default-config > /tmp/uvnc-winvnc-linux-server.conf
```

The generated preview deliberately does not embed production VNC passwords.  Use a private `password_file` or MSLogonII `auth_helper` for real deployments.

## Password file helper

Use `uvnc_winvnc_password_file` to create or validate private VNCAuth password files consumed by `password_file=`. The helper enforces the 8-byte VNCAuth limit and writes files with mode `0600`.

```sh
uvnc_winvnc_password_file --output /etc/ultravnc/vnc-password --password secret
uvnc_winvnc_password_file --validate --output /etc/ultravnc/vnc-password
```

## Runtime tab

The server settings GUI includes a Runtime tab for user-mode start/stop/status/log workflows. It writes the current preview to a runtime config path or a temporary config and starts `uvnc_winvnc_memory_server --config <path> --serve-forever` through `QProcess`. This is the Linux replacement for the basic WinVNC tray/admin live workflow, not a Win32 dialog shim.

## DSM provider

The settings GUI exposes `dsm_provider` for the native Linux DSM/SecureVNC provider ABI. See `docs/native-linux-dsm-provider-abi.md`. Legacy Windows `.dsm` DLL loading is still rejected.
