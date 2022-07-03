find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_link_options(--whole-archive)
add_library(audiomixer_service_proto audiomixer_service.proto)
target_link_libraries(audiomixer_service_proto
  PUBLIC
  protobuf::libprotobuf
  gRPC::grpc
  gRPC::grpc++
  gRPC::grpc++_reflection
)

target_include_directories(audiomixer_service_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET audiomixer_service_proto LANGUAGE cpp)
protobuf_generate(TARGET audiomixer_service_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")