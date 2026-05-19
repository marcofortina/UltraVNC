# Native Linux server security extensions

The native Linux server must not copy the Windows DSM/MSLogon implementation blindly.
Those paths depend on Windows DLL loading, Windows account/domain APIs, and historical
UltraVNC plugin handshake details that are not safe to expose as a partial Linux port.

Current Linux policy:

- DSM/security plugin command line options are rejected explicitly.
- MSLogon command line options are rejected explicitly.
- The legacy HTTP Java viewer endpoint is rejected explicitly.
- VeNCrypt X.509 + VNCAuth is the supported encrypted Linux path for now.
- Future Linux-native authentication should be designed around explicit providers, for
  example PAM, certificate identity, or a reviewed portable plugin ABI.

The rejection is intentional product behavior. The server must fail closed instead of
starting with a misleading or no-op security plugin configuration.
