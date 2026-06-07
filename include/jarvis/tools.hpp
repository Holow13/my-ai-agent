#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace jarvis {

nlohmann::json tool_definitions();
std::string execute_tool(const std::string& name, const nlohmann::json& arguments);

}  // namespace jarvis
