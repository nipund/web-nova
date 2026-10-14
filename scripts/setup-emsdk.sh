#!/usr/bin/env bash
# Installs the pinned Emscripten SDK into ./.emsdk (idempotent).
# In CI you can skip this and provide EMSDK yourself (e.g. mymindstorm/setup-emsdk
# with the version from .emsdk-version); build.sh honours an existing $EMSDK.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(tr -d '[:space:]' < "$root/.emsdk-version")"
dir="$root/.emsdk"

if [[ ! -d "$dir/.git" ]]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$dir"
fi
"$dir/emsdk" install "$version"
"$dir/emsdk" activate "$version"
echo "emsdk $version ready in $dir"
