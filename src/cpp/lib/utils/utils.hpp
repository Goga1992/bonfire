#pragma once

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>

namespace bon::utils
{

void Assert(bool expr);
void Assert(bool expr, const std::string& err);

int GetEnvIntRequired(const std::string& name);
int GetEnvIntWithDefault(const std::string& name, int def = 0);

std::string GenerateUUID();

std::string GetHostname();

}  // namespace bon::utils