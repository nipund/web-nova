# Satisfies upstream's `find_package(SDL3 CONFIG REQUIRED)` with the Emscripten
# SDL3 port. (The port's own config only exists in the sysroot after the port
# has been built once, which is too late for a clean configure.)
if(NOT TARGET SDL3::SDL3)
  foreach(_t SDL3::Headers SDL3::SDL3 SDL3::SDL3-static)
    add_library(${_t} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${_t} PROPERTIES INTERFACE_COMPILE_OPTIONS "-sUSE_SDL=3")
  endforeach()
  set_target_properties(SDL3::SDL3 SDL3::SDL3-static PROPERTIES
    INTERFACE_LINK_LIBRARIES "-sUSE_SDL=3")
endif()
set(SDL3_FOUND TRUE)
