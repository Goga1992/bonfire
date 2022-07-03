#pragma once

#include "videoscaler_service.grpc.pb.h"
#include "videoscaler_service.pb.h"

#include "videoscaler_manager.hpp"

class VideoScalerService final : public VideoScaler::Service
{
 public:
  VideoScalerService() = default;
  ~VideoScalerService() = default;

 public:
  grpc::Status StartVideoSlot(grpc::ServerContext* context,
                              const StartVideoSlotRequest* request,
                              VideoSlotInfo* response) override;

  grpc::Status SetSlotBranchActive(grpc::ServerContext* context,
                                   const SetSlotBranchActiveRequest* request,
                                   google::protobuf::Empty* response) override;

  grpc::Status ForceKeyFrame(grpc::ServerContext* context,
                             const ForceKeyFrameRequest* request,
                             google::protobuf::Empty* response) override;

  grpc::Status StopVideoSlot(grpc::ServerContext* context,
                             const StopVideoSlotRequest* request,
                             google::protobuf::Empty* response) override;

 private:
  VideoScalerManager manager;
};
