#!/usr/bin/env bash
# This file is part of UltraVNC
# https://github.com/ultravnc/UltraVNC
# https://uvnc.com/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STATUS_DOC="${ROOT_DIR}/docs/native-linux-issue-348-status.md"
VIEWER_DOC="${ROOT_DIR}/docs/native-linux-qt-viewer-production-checklist.md"
SECURITY_DOC="${ROOT_DIR}/docs/native-linux-security-extensions.md"
SERVICE_DOC="${ROOT_DIR}/docs/native-linux-service-admin-equivalents.md"
PR_DOC="${ROOT_DIR}/docs/native-linux-pr-readiness.md"
PR_BODY="${ROOT_DIR}/docs/native-linux-pr-body-template.md"

for path in "${STATUS_DOC}" "${VIEWER_DOC}" "${SECURITY_DOC}" "${SERVICE_DOC}" "${PR_DOC}" "${PR_BODY}"; do
  test -f "${path}"
done

grep -q 'Qt server settings/admin GUI with live start/stop/status/log' "${STATUS_DOC}"
grep -q 'DSM provider ABI and native SecureVNC provider artifact' "${STATUS_DOC}"
grep -q 'MSLogonII viewer auth support' "${STATUS_DOC}"
grep -q 'Add a minimal Qt server/admin/settings shell target' "${STATUS_DOC}"
grep -q 'Move shared server/admin/settings behavior toward the Qt UI layer' "${STATUS_DOC}"
grep -q 'Keep platform-specific integrations isolated under platform backends' "${STATUS_DOC}"

grep -q 'Tight JPEG, Tight gradient, ZlibHex' "${VIEWER_DOC}"
grep -q 'native Linux DSM provider ABI' "${VIEWER_DOC}"
grep -q 'Windows PE/COFF `.dsm` binaries remain rejected' "${VIEWER_DOC}"

grep -q 'Linux-native DSM providers can be loaded' "${SECURITY_DOC}"
grep -q 'SecureVNC is available as a native Linux provider artifact' "${SECURITY_DOC}"
grep -q 'Windows PE/COFF `.dsm` binaries are rejected explicitly' "${SECURITY_DOC}"

grep -q 'Qt live admin/status panel' "${SERVICE_DOC}"
grep -q 'uvnc_settings' "${SERVICE_DOC}"

grep -q 'Recommended PR split' "${PR_DOC}"
grep -q 'Refs #348' "${PR_BODY}"
grep -q 'native Linux DSM provider ABI' "${PR_BODY}"

if grep -R "not-ported-linux-use-status-files-and-journal\|unsupported-fail-closed\|compressed encoding decoders not yet implemented" \
  "${ROOT_DIR}/docs" >/tmp/uvnc-issue348-stale-status.out; then
  cat /tmp/uvnc-issue348-stale-status.out >&2
  echo "stale native Linux status text found" >&2
  exit 1
fi

echo "Issue #348 native Linux status smoke passed."
