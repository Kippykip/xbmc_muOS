# SDL2 windowing platform, for devices without DRM/KMS, X11 or Wayland
# (e.g. Allwinner H700 handhelds running muOS with the libmali fbdev driver).
list(APPEND PLATFORM_REQUIRED_DEPS SDL2)

if(APP_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES EGL)
else()
  message(FATAL_ERROR "The sdl platform only supports APP_RENDER_SYSTEM=gles")
endif()
