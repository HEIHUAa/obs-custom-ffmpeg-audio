cmake_minimum_required(VERSION 3.24...3.25)

project(obs-custom-ffmpeg-audio
  VERSION 1.0.0.0
  LANGUAGES CXX
)

if(NOT PROJECT_IS_TOP_LEVEL)
  if(UNIX)
    add_compile_options(-Wno-error)
  elseif(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_WARNING_AS_ERROR OFF)
  endif()
endif()

add_subdirectory(obs-custom-ffmpeg-audio)