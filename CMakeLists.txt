cmake_minimum_required(VERSION 3.25)

project(webnova LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR
    "webnova only builds for WebAssembly. Use `scripts/build.sh` (or the "
    "`wasm-release` CMake preset with the Emscripten toolchain file).")
endif()

set(EVNOVA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/evnova-decomp"
    CACHE PATH "Checkout of wraitii/evnova-decomp (git submodule)")
if(NOT EXISTS "${EVNOVA_DIR}/src/CMakeLists.txt")
  message(FATAL_ERROR
    "${EVNOVA_DIR} is empty. Run: git submodule update --init --recursive")
endif()

option(WEBNOVA_JSPI
  "Suspend the blocking game loop with JSPI (smaller/faster, needs a recent \
browser) instead of ASYNCIFY" ON)

# ---------------------------------------------------------------------------
# Upstream configuration. Upstream's top-level CMakeLists.txt is bypassed (it
# is built around vcpkg and native toolchains), so the knobs that
# src/CMakeLists.txt reads are set here.
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The probe harness is a POSIX socket server; it cannot exist in a browser.
set(EVNOVA_ENABLE_PROBE OFF)
# A newer clang than upstream develops against; do not fail on new warnings.
set(EVNOVA_WARNINGS_AS_ERRORS OFF)
set(EVNOVA_ENABLE_CLANG_TIDY OFF)
set(EVNOVA_ENABLE_PCH ON)

# -O2 is the practical balance of wasm size and link time (CMake's -O3 default
# buys little for this game and slows the link considerably).
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# C++ exceptions are used by the game (top-level catch, std::filesystem, ...).
# Native wasm exceptions are far cheaper than the JS-based emulation.
add_compile_options(-fwasm-exceptions)
add_link_options(-fwasm-exceptions)

# Dependencies: SDL3 comes from Emscripten's own CMake config; SDL3_ttf and
# zlib are mapped onto Emscripten ports by the shims; fmt is built from source.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/shims")
list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/shims")

include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.1.0
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(fmt)

find_package(SDL3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_subdirectory("${EVNOVA_DIR}/src" evnova-src)

# src/CMakeLists.txt adds ${PROJECT_SOURCE_DIR}/src, which is this project's
# (nonexistent) src; point it at the real one.
target_include_directories(evnova_runtime PUBLIC "${EVNOVA_DIR}/src")

# Browser replacements for platform pieces SDL3 lacks on Emscripten.
target_sources(evnova_runtime PRIVATE web/sdl_dialog_web.cpp)
set_source_files_properties(web/sdl_dialog_web.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# ---------------------------------------------------------------------------
# Web packaging
# ---------------------------------------------------------------------------
set(WEBNOVA_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")

# The page's UI modules (Preact components, see web/ui/) are served as-is next
# to index.html; re-copied on every build so edits show up without a relink.
add_custom_target(webnova_ui ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/web/ui" "${CMAKE_BINARY_DIR}/site/ui")

set(_async_flag -sASYNCIFY=1)
if(WEBNOVA_JSPI)
  set(_async_flag -sJSPI=1)
endif()

set_target_properties(evnova PROPERTIES
  SUFFIX ".html"
  OUTPUT_NAME "index"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
  LINK_DEPENDS "${WEBNOVA_SHELL}")

target_link_options(evnova PRIVATE
  ${_async_flag}
  "--shell-file=${WEBNOVA_SHELL}"
  # The shell loads game data into the virtual filesystem before main() runs.
  -sINVOKE_RUN=0
  -sEXIT_RUNTIME=1
  -sFORCE_FILESYSTEM=1
  -lidbfs.js
  "-sEXPORTED_RUNTIME_METHODS=['callMain','FS','IDBFS','ccall']"
  -sALLOW_MEMORY_GROWTH=1
  -sINITIAL_MEMORY=268435456
  -sMAXIMUM_MEMORY=4GB
  -sSTACK_SIZE=5242880
  -sENVIRONMENT=web
  -sMIN_WEBGL_VERSION=2
  -sMAX_WEBGL_VERSION=2)

