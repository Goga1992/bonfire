#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using RoomId = std::string;
using SlotId = std::string;

struct SlotConfig
{
  std::string sink_hostname;
  std::vector<uint32_t> sink_ports;
};

struct SlotInfo
{
  std::string id;
  uint32_t port;
};

struct VideoResolution
{
  std::string_view name;
  int width;
  int height;
};