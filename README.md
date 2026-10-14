# webnova

[![Live site](https://img.shields.io/badge/play-nipund.github.io%2Fweb--nova-a61814)](https://nipund.github.io/web-nova/)
[![Deploy](https://img.shields.io/github/actions/workflow/status/nipund/web-nova/pages.yml?branch=main&label=deploy)](https://github.com/nipund/web-nova/actions/workflows/pages.yml)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Emscripten-654ff0?logo=webassembly&logoColor=white)](https://emscripten.org/)

Build of [wraitii/evnova-decomp](https://github.com/wraitii/evnova-decomp) for the browser with Emscripten.
The upstream repo is a git submodule in `extern/evnova-decomp` and is not
modified. Everything web-specific lives here.

## Build and run

```sh
git clone --recurse-submodules <this repo>     # or: git submodule update --init
scripts/build.sh                               # installs emsdk into .emsdk on first run
scripts/serve.py                               # http://localhost:8080
```

Needs `cmake` (3.25+), `ninja`, `git`, and Python 3. Output is
`build/wasm-release/site/{index.html,index.js,index.wasm,ui/}`; `wasm-debug` is the
other preset (`scripts/build.sh wasm-debug`).

## Game data

The game data is not part of the build and is never fetched by the page. The user
supplies the EV Nova Community Edition zip from their own filesystem. The page
unpacks the files the engine needs into the in-browser filesystem (`/EV Nova`) and
persists them in IndexedDB, so later visits go straight to "Play". Pilots and
preferences persist the same way.

## Notes

- **Async loop:** the engine is a blocking loop with `SDL_Delay(16)`. It runs unchanged
  by suspending in `SDL_Delay`: JSPI by default, or `-DWEBNOVA_JSPI=OFF` for ASYNCIFY.
- **Exceptions:** built with `-fwasm-exceptions`.
- The probe harness is disabled; the game is built with
  `EVNOVA_WARNINGS_AS_ERRORS=OFF`.
- Emscripten ports are prebuilt by `scripts/build.sh` (`embuilder`), because parallel
  ninja jobs otherwise race on the port cache lock on a cold cache.

- **UI:** in-page dialogs are Preact + htm components in `web/ui/` (ES modules, pinned in `web/ui/deps.js`). `web/ui/fs.js` holds
  the virtual-filesystem access, and `sdl_dialog_web.cpp` just calls `window.novaUI`.

## Licensing

The engine is MIT-licensed upstream. No game assets are in this repo.