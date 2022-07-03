#pragma once

#include "audiomixer_manager.hpp"
#include "audiomixer_service.grpc.pb.h"
#include "audiomixer_service.pb.h"

#include <memory>

class MixerService : public AudioMixer::Service
{
 public:
  MixerService() = default;
  ~MixerService() = default;

  virtual grpc::Status StartAudioSlot(grpc::ServerContext* context,
                                      const StartAudioSlotRequest* request,
                                      AudioSlotInfo* response) override;

  virtual grpc::Status StopAudioSlot(grpc::ServerContext* context,
                                const StopAudioSlotRequest* request,
                                google::protobuf::Empty* response) override;

 private:
  MixerManager mixer_manager;
};