#!/usr/bin/env bash
# Configure and build the browser/wasm game.
#   scripts/build.sh [preset]      preset: wasm-release (default) | wasm-debug
# Output: build/<preset>/site/ (index.html, index.js, index.wasm); serve with scripts/serve.py
# copies the deployable files to dist/.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-wasm-release}"
cd "$root"

if [[ ! -f extern/evnova-decomp/src/CMakeLists.txt ]]; then
  git submodule update --init --recursive
fi

if [[ -z "${EMSDK:-}" ]]; then
  [[ -d .emsdk ]] || scripts/setup-emsdk.sh
  # shellcheck disable=SC1091
  source .emsdk/emsdk_env.sh >/dev/null
fi

want="$(tr -d '[:space:]' < .emsdk-version)"
have="$(emcc --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
[[ "$have" == "$want" ]] || echo "warning: emcc $have, but .emsdk-version pins $want" >&2

# Build the Emscripten ports up front: when ninja's parallel compile jobs
# build them on demand they race on the cache lock.
embuilder build sdl3 sdl3_ttf freetype harfbuzz zlib

cmake --preset "$preset"
cmake --build --preset "$preset"
