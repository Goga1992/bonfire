#include "mixer_service.hpp"

#include "types.hpp"

grpc::Status VideoMixerService::StartVideoSlot(grpc::ServerContext* context,
                                               const StartVideoSlotRequest* request,
                                               VideoSlotInfo* response)
{
  SlotConfig cfg
  {
    .room_id=request->room_id(),
    .callback_hostname=request->sink_hostname(),
    .callback_port=request->sink_port(),
  };

  try
  {
    auto info = mixer_manager.AddSlot(cfg);
    response->set_id(info.id);
    response->set_hostname(std::string(""));
    if (info.port.has_value())
    {
      response->set_port(info.port.value());
    }
  }
  catch (const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}

grpc::Status VideoMixerService::StopVideoSlot(grpc::ServerContext* context,
                                              const StopVideoSlotRequest* request,
                                              google::protobuf::Empty* response)
{
  return grpc::Status::OK;
}