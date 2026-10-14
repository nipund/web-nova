# Satisfies upstream's `find_package(ZLIB CONFIG REQUIRED)` with the
# Emscripten zlib port.
if(NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
  set_target_properties(ZLIB::ZLIB PROPERTIES
    INTERFACE_COMPILE_OPTIONS "-sUSE_ZLIB=1"
    INTERFACE_LINK_LIBRARIES "-sUSE_ZLIB=1")
endif()
set(ZLIB_FOUND TRUE)
