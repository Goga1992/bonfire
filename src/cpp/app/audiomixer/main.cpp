#include "logger/logger.hpp"
#include "sync/lock_guarded.hpp"
#include "utils/utils.hpp"

#include "audiomixer_service.hpp"

#include <grpc/grpc.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <csignal>
#include <thread>

using namespace std::chrono_literals;

std::function<void(int)> shutdown_setter;
void signal_handler(int signal) { shutdown_setter(signal); }

int main()
{
  gst_init(nullptr, nullptr);


  MixerService service;

  grpc::ServerBuilder builder;
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  builder.AddListeningPort("0.0.0.0:7001", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

  std::atomic<bool> shutdown = false;

  shutdown_setter = [&](int signal)
  {
    shutdown = true;
  };

  auto shutdown_watcher = std::thread([&]
  {
    while (not shutdown)
    {
      std::this_thread::sleep_for(1s);
    }
    bon::log::Info("Shutting down");
    server->Shutdown();
  });

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  std::signal(SIGQUIT, signal_handler);

  server->Wait();

  shutdown_watcher.join();

  return 0;
}