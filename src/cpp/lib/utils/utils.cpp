#include "utils.hpp"

#include <boost/asio/ip/host_name.hpp>

#include "logger/logger.hpp"

namespace bon::utils
{

void Assert(bool expr)
{
  if (not expr)
  {
    bon::log::Critical("Assert failed");
    abort();
  }
}

void Assert(bool expr, const std::string& err)
{
  if (not expr)
  {
    bon::log::Critical("Assert failed: {}", err);
    abort();
  }
}

int GetEnvIntRequired(const std::string& name)
{
  const char* value = std::getenv(name.c_str());
  Assert(value != nullptr, fmt::format("Env is required: name=[{}]", name));
  return std::atoi(value);
}

int GetEnvIntWithDefault(const std::string& name, int def)
{
  const char* value = std::getenv(name.c_str());
  return value == nullptr ? def : std::atoi(value);
}

std::string GenerateUUID()
{
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  return boost::uuids::to_string(uuid);
}

std::string GetHostname()
{
  return boost::asio::ip::host_name();
}

}