#include "videoscaler_service.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include "types.hpp"

grpc::Status VideoScalerService::StartVideoSlot(grpc::ServerContext* context,
                                               const StartVideoSlotRequest* request,
                                               VideoSlotInfo* response)
{
  bon::log::Info("StartVideoSlot request: sink_hostname=[{}], sink_ports=[{}]",
                 request->sink_hostname(),
                 fmt::to_string(fmt::join(request->sink_ports(), ",")));

  if (request->sink_ports().size() != std::size(VideoScalerSlot::RESOLUTIONS))
  {
    return {grpc::StatusCode::INVALID_ARGUMENT, "not enough sink ports"};
  }

  SlotConfig cfg
  {
    .sink_hostname=request->sink_hostname(),
    .sink_ports={request->sink_ports().begin(), request->sink_ports().end()},
  };

  try
  {
    auto info = manager.StartVideoSlot(cfg);
    response->set_id(info.id);
    response->set_hostname(bon::utils::GetHostname());
    response->set_port(info.port);
  }
  catch (const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}

grpc::Status VideoScalerService::SetSlotBranchActive(grpc::ServerContext* context,
                                                     const SetSlotBranchActiveRequest* request,
                                                     google::protobuf::Empty* response)
{
  bon::log::Info("SetSlotBranchActive request: slot_id=[{}], name=[{}], active=[{}]",
                  request->slot_id(), request->name(), request->active());

  try
  {
    manager.SetSlotBranchActive(request->slot_id(),
                                request->name(),
                                request->active());
  }
  catch (const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}

grpc::Status VideoScalerService::ForceKeyFrame(grpc::ServerContext* context,
                                               const ForceKeyFrameRequest* request,
                                               google::protobuf::Empty* response)
{
  bon::log::Info("ForceKeyFrame request: slot_id=[{}], name=[{}]",
                  request->slot_id(), request->name());

  try
  {
    manager.ForceKeyFrame(request->slot_id(), request->name());
  }
  catch (const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}

grpc::Status VideoScalerService::StopVideoSlot(grpc::ServerContext* context,
                                              const StopVideoSlotRequest* request,
                                              google::protobuf::Empty* response)
{
  bon::log::Info("StopVideoSlot request: slot_id=[{}]",
                  request->slot_id());

  try
  {
    manager.StopVideoSlot(request->slot_id());
  }
  catch(const std::exception& e)
  {
    return {grpc::StatusCode::INTERNAL, e.what()};
  }

  return grpc::Status::OK;
}