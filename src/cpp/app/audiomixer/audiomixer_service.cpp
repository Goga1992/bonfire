#include "audiomixer_service.hpp"

#include <logger/logger.hpp>
#include <utils/utils.hpp>

grpc::Status MixerService::StartAudioSlot(grpc::ServerContext* context,
                                          const StartAudioSlotRequest* request,
                                          AudioSlotInfo* response)
{
  SlotConfig cfg
  {
    .room_id=request->room_id(),
    .sink_hostname=request->sink_hostname(),
    .sink_port=request->sink_port(),
    .listener=request->listener(),
  };

  try
  {
    auto info = mixer_manager.AddSlot(cfg);
    response->set_id(info.id);
    response->set_hostname(bon::utils::GetHostname());
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

grpc::Status MixerService::StopAudioSlot(grpc::ServerContext* context, const StopAudioSlotRequest* request, google::protobuf::Empty* response)
{
  try
  {
    mixer_manager.RemoveSlot(request->slot_id());
  }
  catch (const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}
