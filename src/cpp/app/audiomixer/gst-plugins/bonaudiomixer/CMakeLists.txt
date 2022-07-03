include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LIBNAME "bonaudiomixer")

add_library(
  ${LIBNAME}
  SHARED

  gstaudiomixer.c
  gstaudiomixerelement.c
  gstaudiomixerplugin.c
  gstaudiomixerorc.c # generated
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.h
  COMMAND orcc --header -o ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.h ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.orc
  VERBATIM
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.c
  COMMAND orcc --implementation -o ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.c ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.orc
  VERBATIM
)

add_custom_target(
  gstaudiomixerorc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.h ${CMAKE_CURRENT_SOURCE_DIR}/gstaudiomixerorc.c
)

add_dependencies(${LIBNAME} gstaudiomixerorc)

set_target_properties(${LIBNAME} PROPERTIES LINKER_LANGUAGE C)
add_definitions(-DPACKAGE="bonaudiomixer" -DHAVE_CONFIG_H)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0>=1.4)
pkg_check_modules(GST_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)
target_include_directories(${LIBNAME} PUBLIC ${GST_INCLUDE_DIRS}  /usr/local/include/orc-0.4)

target_compile_options(${LIBNAME} PRIVATE ${GST_CFLAGS})
target_link_libraries(${LIBNAME} ${GST_LIBRARIES} ${GST_BASE_LIBRARIES} ${GST_AUDIO_LIBRARIES} orc-0.4)
