#pragma once

#include "mixer_service.grpc.pb.h"
#include "mixer_service.pb.h"

#include "mixer_manager.hpp"

class VideoMixerService final : public VideoMixer::Service
{
 public:
  VideoMixerService() = default;
  ~VideoMixerService() = default;

 public:
  grpc::Status StartVideoSlot(grpc::ServerContext* context,
                              const StartVideoSlotRequest* request,
                              VideoSlotInfo* response) override;

  grpc::Status StopVideoSlot(grpc::ServerContext* context,
                             const StopVideoSlotRequest* request,
                             google::protobuf::Empty* response) override;

 private:
  VideoMixerManager mixer_manager;
};
