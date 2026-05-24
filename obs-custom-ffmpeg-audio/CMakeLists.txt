add_library(obs-custom-ffmpeg-audio MODULE)

find_package(libobs CONFIG)
if(NOT TARGET FFmpeg::avcodec)
  find_package(FFmpeg REQUIRED
    COMPONENTS avcodec avutil)
endif()

target_sources(
  obs-custom-ffmpeg-audio
  PRIVATE
  obs-custom-ffmpeg-audio-plugin-main.cpp
  obs-custom-ffmpeg-audio-encoder.cpp
  obs-custom-ffmpeg-audio-encoder.hpp
)

target_link_libraries(obs-custom-ffmpeg-audio PRIVATE OBS::libobs FFmpeg::avcodec FFmpeg::avutil)

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