#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include "mixer_service.hpp"
#include "mixer_slot.hpp"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

int main()
{
  gst_init(nullptr, nullptr);

  bon::log::Info("Okay...{}, {}", "shpekis", 12312);

  VideoMixerService service;

  grpc::ServerBuilder builder;
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  builder.AddListeningPort("0.0.0.0:7000", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  server->Wait();

  return 0;
}