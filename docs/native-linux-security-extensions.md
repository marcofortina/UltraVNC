# Native Linux server security extensions

The native Linux server must not copy the Windows DSM/MSLogon implementation blindly.
Those paths depend on Windows DLL loading, Windows account/domain APIs, and historical
UltraVNC plugin handshake details that are not safe to expose as a partial Linux port.

Current Linux server policy:

- DSM/security plugin command line options are rejected explicitly.
- SecureVNC is treated as a DSM plugin and rejected explicitly.
- MSLogon I remains legacy-only. MSLogonII is available on the native Linux
  server through the explicit external auth helper described in
  `native-linux-mslogon-server-auth.md`; it is not a Windows SSPI/domain API port.
- The legacy HTTP Java applet viewer endpoint is rejected explicitly; see `native-linux-http-java-viewer-legacy.md`.
- VeNCrypt X.509 + VNCAuth is the supported encrypted Linux server path for now.
- Future Linux-native server authentication should be designed around explicit
  providers, for example PAM, certificate identity, or a reviewed portable plugin ABI.

Current portable viewer policy:

- MSLogonII can now be negotiated by the portable viewer when requested with
  `--security-extension mslogon`, `--username` and a password. This is for
  compatibility with original UltraVNC servers that offer `rfbUltraVNC_MsLogonIIAuth`.
- MSLogon I remains legacy-only.
- DSM/SecureVNC plugin stream transforms are still not implemented in the portable
  Linux viewer path because they require the DSMPlugin stream ABI, not just a
  security-type number.

The rejection is intentional product behavior. The server must fail closed instead of
starting with a misleading or no-op security plugin configuration.

## Native Linux MSLogonII server auth

MSLogonII server authentication is available through an explicit external helper. See `docs/native-linux-mslogon-server-auth.md`. DSM and SecureVNC remain fail-closed plugin features.

## Native Linux DSM provider ABI

A Linux shared-object provider ABI exists for DSM/SecureVNC-style stream transforms. See `docs/native-linux-dsm-provider-abi.md`. The old Windows `.dsm` DLL ABI remains rejected.
