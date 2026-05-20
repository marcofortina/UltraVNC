# Native Linux PR readiness

This document defines the local evidence expected before opening the first #348 native Linux PR.

## Recommended PR split

The branch is large enough that maintainers may prefer staged PRs:

1. Linux build foundation and headless repeater.
2. Portable WinVNC/RFB core and memory server.
3. Native Linux server runtime, capture/input backends and security hardening.
4. Native Linux Qt viewer.
5. Native Linux Qt server settings/admin and legacy executable parity.
6. Security extensions: MSLogonII helper, DSM provider ABI and native SecureVNC provider.

If the maintainer accepts one large PR, the PR body should explicitly say that the work is a milestone branch and not a one-patch change.

## Local evidence before opening PR

Run:

```sh
cmake/native-linux-core-parity-smoke.sh \
  /tmp/uvnc-native-linux-core-parity-build \
  /tmp/uvnc-native-linux-core-parity-install

cmake/native-linux-legacy-parity-smoke.sh \
  /tmp/uvnc-native-linux-legacy-parity-build \
  /tmp/uvnc-native-linux-legacy-parity-install

cmake/native-linux-issue-348-status-smoke.sh
```

Attach or paste the resulting green logs to the PR summary.

## Still not claimed by local gates

The local gates do not replace:

- GitHub Actions result on the opened PR;
- external viewer/server matrix evidence for x11vnc, LibVNCServer, RealVNC and UltraVNC server;
- human visual review of the Qt parity snapshot.

Those should remain follow-up evidence unless the corresponding logs/screenshots are attached.
