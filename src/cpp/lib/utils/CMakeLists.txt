bon_add_library(bon_utils)

find_package(Boost COMPONENTS system REQUIRED)

message(${Boost_INCLUDE_DIRS})
target_include_directories(bon_utils PUBLIC ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(bon_utils bon_logger ${Boost_LIBRARIES})