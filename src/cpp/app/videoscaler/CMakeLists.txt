bon_add_executable(videoscaler)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0>=1.4)
pkg_check_modules(GST_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)

target_include_directories(videoscaler
  PUBLIC

  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_BINARY_DIR}/proto

  ${GST_INCLUDE_DIRS}
)

target_link_libraries(videoscaler
  bon_logger
  bon_utils
  videoscaler_service_proto

  ${GST_LIBRARIES} ${GST_BASE_LIBRARIES} ${GST_AUDIO_LIBRARIES}
)

add_subdirectory(proto)