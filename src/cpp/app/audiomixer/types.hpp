#pragma once

#include <cstddef>
#include <optional>
#include <string>

using RoomId = std::string;
using SlotId = std::string;

struct SlotConfig
{
  RoomId room_id;
  std::string sink_hostname;
  uint32_t sink_port;
  bool listener = false;
};

struct SlotInfo
{
  std::string id;
  std::optional<uint32_t> port = std::nullopt;
};
