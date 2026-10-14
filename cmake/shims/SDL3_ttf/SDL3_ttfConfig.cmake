# Satisfies upstream's `find_package(SDL3_ttf CONFIG REQUIRED)` with the
# Emscripten port (-sUSE_SDL_TTF=3 pulls in SDL3, FreeType and HarfBuzz).
if(NOT TARGET SDL3_ttf::SDL3_ttf)
  add_library(SDL3_ttf::SDL3_ttf INTERFACE IMPORTED GLOBAL)
  set_target_properties(SDL3_ttf::SDL3_ttf PROPERTIES
    INTERFACE_COMPILE_OPTIONS "-sUSE_SDL_TTF=3"
    INTERFACE_LINK_LIBRARIES "-sUSE_SDL_TTF=3")
endif()
set(SDL3_ttf_FOUND TRUE)
