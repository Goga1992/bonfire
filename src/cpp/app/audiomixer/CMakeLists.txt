bon_add_executable(audiomixer)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0>=1.4)
pkg_check_modules(GST_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)

target_include_directories(audiomixer
  PUBLIC

  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_BINARY_DIR}/proto

  ${GST_INCLUDE_DIRS}
)

target_link_libraries(audiomixer
  bon_logger
  bon_utils
  audiomixer_service_proto
  bonaudiomixer

  ${GST_LIBRARIES} ${GST_BASE_LIBRARIES} ${GST_AUDIO_LIBRARIES}
)

add_subdirectory(proto)

add_subdirectory(gst-plugins)