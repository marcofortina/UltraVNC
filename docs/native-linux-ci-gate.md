# Native Linux CI gate

This document describes the CI-safe gate for the native Linux support track.
It is a local and GitHub Actions validation gate, not a claim that full native
Linux server/viewer production certification is complete.

## Scope

The gate is intentionally limited to checks that can run on a generic Ubuntu CI
runner without requiring a live graphical desktop, live input injection, external
VNC products or a persistent systemd user session.

The gate covers:

- Linux headless repeater smoke coverage.
- WinVNC portable core closure coverage.
- memory-server RFB closure coverage.
- X11 capture build/runtime availability checks.
- PipeWire/XDG portal detection/scaffolding checks.
- Linux input backend and XTest availability checks.
- Linux server integration template/install checks.
- Linux server real-runtime checks in CI-safe mode.
- Linux server secure VNCAuth runtime checks in CI-safe mode.
- Linux server negative runtime/failure-mode checks.
- Linux user-service template checks in non-invasive mode.
- Qt viewer startup/RFB/persistent-input/compressed-encoding smoke coverage.

## Local command

Run the same aggregate gate used by the workflow:

```bash
cmake/native-linux-ci-gate-smoke.sh \
  /tmp/uvnc-native-linux-ci-gate-build \
  /tmp/uvnc-native-linux-ci-gate-install
```

## Live checks intentionally outside CI

These checks require a prepared local machine and must not be silently folded into
headless CI:

```bash
UVNC_RUN_REAL_X11_SERVER=1 cmake/linux-server-real-runtime-smoke.sh \
  /tmp/uvnc-linux-server-real-runtime-build \
  /tmp/uvnc-linux-server-real-runtime-install
```

```bash
UVNC_RUN_SECURE_X11_SERVER=1 cmake/linux-server-secure-runtime-smoke.sh \
  /tmp/uvnc-linux-server-secure-runtime-build \
  /tmp/uvnc-linux-server-secure-runtime-install
```

```bash
UVNC_RUN_SYSTEMD_USER_SERVICE=1 \
UVNC_OVERWRITE_USER_SERVICE_CONFIG=1 \
cmake/linux-server-user-service-smoke.sh \
  /tmp/uvnc-linux-server-user-service-build \
  /tmp/uvnc-linux-server-user-service-install
```

The server compatibility matrix with TigerVNC, LibVNC, RealVNC and the Windows
UltraVNC viewer is also intentionally outside this CI gate because those clients
must be installed and licensed/available explicitly.

## GitHub Actions workflow

The `Native Linux subset` workflow runs the aggregate gate from
`.github/workflows/native-linux-subset.yml`.

The workflow can be triggered manually with `workflow_dispatch` or by the usual
push/pull-request events on `main`/`master`.

## Deferred PR work

PR creation, final PR body wording and inspection of GitHub check results are
deferred until the end of the current native Linux hardening pass.

Until a real PR exists and the workflow has completed there, do not claim that
GitHub CI is green. The local gate can only be reported as local validation.
