#.rst:
# FindSDL2
# --------
# Finds the SDL2 library (used by the "sdl" windowing platform)
#
# This will define the following target:
#
#   SDL2::SDL2   - The SDL2 library

if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SDL2 sdl2 QUIET)
  endif()

  find_path(SDL2_INCLUDE_DIR NAMES SDL.h
                             PATH_SUFFIXES SDL2
                             HINTS ${PC_SDL2_INCLUDEDIR} ${PC_SDL2_INCLUDE_DIRS}
                             NO_CACHE)
  find_library(SDL2_LIBRARY NAMES SDL2 SDL2-2.0
                            HINTS ${PC_SDL2_LIBDIR}
                            NO_CACHE)

  set(SDL2_VERSION ${PC_SDL2_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SDL2
                                    REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR
                                    VERSION_VAR SDL2_VERSION)

  if(SDL2_FOUND)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
                                     IMPORTED_LOCATION "${SDL2_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS "HAVE_SDL=1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP SDL2::SDL2)
  endif()
endif()
