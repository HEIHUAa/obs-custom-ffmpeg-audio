add_library(obs-custom-ffmpeg-audio MODULE)

find_package(libobs CONFIG)

find_package(FFmpeg REQUIRED
  COMPONENTS avcodec avutil)

target_sources(
  obs-custom-ffmpeg-audio
  PRIVATE
  obs-custom-ffmpeg-audio-plugin-main.cpp
  obs-custom-ffmpeg-audio-encoder.cpp
  obs-custom-ffmpeg-audio-encoder.hpp
)

if(WIN32)
  foreach(_comp avcodec avutil)
    get_target_property(_type FFmpeg::${_comp} TYPE)
    get_target_property(_implib FFmpeg::${_comp} IMPORTED_IMPLIB)
    get_target_property(_location FFmpeg::${_comp} IMPORTED_LOCATION)
    message(STATUS "FFmpeg::${_comp}: TYPE=${_type} IMPLIB=${_implib} LOCATION=${_location}")
  endforeach()

  foreach(_comp avcodec avutil)
    if(NOT FFmpeg_${_comp}_IMPLIB MATCHES "-NOTFOUND$" AND FFmpeg_${_comp}_IMPLIB)
      set(_ffmpeg_${_comp}_lib "${FFmpeg_${_comp}_IMPLIB}")
    else()
      get_filename_component(_ffmpeg_hint "${FFmpeg_avcodec_INCLUDE_DIR}" DIRECTORY)
      find_library(_ffmpeg_${_comp}_lib
        NAMES ${_comp} lib${_comp}
        HINTS "${_ffmpeg_hint}/lib"
              "${_ffmpeg_hint}/bin"
              ${CMAKE_PREFIX_PATH}
      )
      if(NOT _ffmpeg_${_comp}_lib)
        find_library(_ffmpeg_${_comp}_lib
          NAMES ${_comp} lib${_comp}
        )
      endif()
    endif()
    if(NOT _ffmpeg_${_comp}_lib OR _ffmpeg_${_comp}_lib MATCHES "-NOTFOUND$")
      message(FATAL_ERROR "Cannot find FFmpeg ${_comp} library. "
        "FFmpeg_${_comp}_IMPLIB='${FFmpeg_${_comp}_IMPLIB}' "
        "CMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATH}' "
        "Hints tried: ${_ffmpeg_hint}/lib, ${_ffmpeg_hint}/bin")
    endif()
    message(STATUS "Linking FFmpeg ${_comp}: ${_ffmpeg_${_comp}_lib}")
  endforeach()

  target_link_libraries(obs-custom-ffmpeg-audio PRIVATE
    OBS::libobs
    FFmpeg::avcodec
    FFmpeg::avutil
  )
  target_link_libraries(obs-custom-ffmpeg-audio PRIVATE
    "${_ffmpeg_avcodec_lib}"
    "${_ffmpeg_avutil_lib}"
  )
else()
  target_link_libraries(obs-custom-ffmpeg-audio PRIVATE OBS::libobs FFmpeg::avcodec FFmpeg::avutil)
endif()

set_target_properties(obs-custom-ffmpeg-audio PROPERTIES
  FOLDER "plugins/obs-custom-ffmpeg-audio"
  PREFIX ""
)

target_compile_features(obs-custom-ffmpeg-audio PRIVATE cxx_std_20)

if(WIN32)
  target_compile_options(obs-custom-ffmpeg-audio PRIVATE
    $<$<CONFIG:Release>:/O1>
    $<$<CONFIG:Release>:/Gw>
    $<$<CONFIG:Release>:/Zc:inline>
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Release>:/Oi>
    /MP
    $<$<CONFIG:Release>:/GL>
    $<$<CONFIG:Release>:/Gy>
    $<$<CONFIG:Release>:/GR->
  )
  target_link_options(obs-custom-ffmpeg-audio PRIVATE
    $<$<CONFIG:Release>:/LTCG>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
    /INCREMENTAL:NO
  )
endif()

install(
  TARGETS obs-custom-ffmpeg-audio
  RUNTIME DESTINATION "obs-plugins/64bit"
  LIBRARY DESTINATION "obs-plugins"
)

install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/locale/en-US.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/data/locale/zh-CN.ini
  DESTINATION "data/obs-plugins/obs-custom-ffmpeg-audio/locale"
)