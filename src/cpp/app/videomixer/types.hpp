#pragma once

#include <cstddef>
#include <optional>
#include <string>

using RoomId = std::string;
using SlotId = std::string;

struct SlotConfig
{
  RoomId room_id;
  std::string callback_hostname;
  uint32_t callback_port;
  uint32_t ssrc;
};

struct SlotInfo
{
  std::string id;
  std::optional<uint32_t> port = std::nullopt;
};
