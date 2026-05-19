#!/usr/bin/env sh
set -eu
for alias_path in "$@"; do
    if [ ! -e "$alias_path" ] && [ ! -L "$alias_path" ]; then
        echo "missing legacy executable alias: $alias_path" >&2
        exit 1
    fi
    if [ ! -x "$alias_path" ]; then
        echo "legacy executable alias is not executable: $alias_path" >&2
        exit 1
    fi
done
