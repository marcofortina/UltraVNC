# Native Linux MSLogonII server authentication

The native Linux server supports UltraVNC MSLogonII at the RFB security-type layer through an explicit external authentication helper.

This is intentionally not a blind port of the Windows SSPI/domain-account implementation.  The Linux trust boundary is a local helper executable chosen by the operator.  The server performs the MSLogonII Diffie-Hellman credential exchange, decrypts the username and password from the viewer, and invokes the helper with environment variables:

```text
UVNC_AUTH_METHOD=mslogon-ii
UVNC_AUTH_USERNAME=<viewer username>
UVNC_AUTH_PASSWORD=<viewer password>
```

The helper must exit with status `0` to accept the login.  Any non-zero exit status rejects the RFB authentication and the server sends the normal RFB auth failure result.

Example configuration:

```text
auth=mslogon-ii
auth_helper=/usr/local/libexec/uvnc-mslogon-auth
```

Equivalent CLI:

```sh
uvnc_winvnc_memory_server \
  --auth mslogon-ii \
  --auth-helper /usr/local/libexec/uvnc-mslogon-auth \
  --transport-security vencrypt-x509-vnc \
  --tls-cert-file /etc/ultravnc/server.crt \
  --tls-key-file /etc/ultravnc/server.key
```

Operational rules:

- the helper path must be absolute;
- credentials are not passed on the command line;
- helpers should be owned by root or the service account and not group/world writable;
- production deployments should still use transport encryption because MSLogonII only protects the authentication exchange, not framebuffer/input traffic;
- MSLogon I remains legacy and is not implemented as a native Linux server auth mode.

DSM/SecureVNC remain plugin-stream-transform features and are not implemented by this helper.
