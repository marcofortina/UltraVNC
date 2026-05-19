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
