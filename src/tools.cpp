#include "jarvis/tools.hpp"

#include "jarvis/utils.hpp"

#include <array>
#include <chrono>
#include <cstdio>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace jarvis {

namespace {

bool is_safe_command(const std::string& command) {
  static const std::array<const char*, 8> allowed_prefixes = {
      "ls", "pwd", "echo", "cat", "head", "tail", "grep", "find",
  };

  const std::string trimmed = trim(command);
  if (trimmed.empty()) {
    return false;
  }

  const std::string lower = to_lower(trimmed);
  for (const auto* prefix : allowed_prefixes) {
    if (lower == prefix || lower.rfind(std::string(prefix) + " ", 0) == 0) {
      return true;
    }
  }
  return false;
}

std::string tool_get_time() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm{};
#if defined(_WIN32)
  localtime_s(&local_tm, &time);
#else
  localtime_r(&time, &local_tm);
#endif

  std::ostringstream stream;
  stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return stream.str();
}

std::string tool_read_file(const nlohmann::json& arguments) {
  const std::string path = arguments.at("path").get<std::string>();
  if (!fs::exists(path) || !fs::is_regular_file(path)) {
    throw std::runtime_error("File not found: " + path);
  }
  return read_file(path);
}

std::string tool_list_directory(const nlohmann::json& arguments) {
  const std::string path = arguments.at("path").get<std::string>();
  if (!fs::exists(path) || !fs::is_directory(path)) {
    throw std::runtime_error("Directory not found: " + path);
  }

  std::ostringstream result;
  for (const auto& entry : fs::directory_iterator(path)) {
    result << entry.path().filename().string();
    if (entry.is_directory()) {
      result << "/";
    }
    result << '\n';
  }
  return result.str();
}

std::string tool_run_command(const nlohmann::json& arguments) {
  const std::string command = arguments.at("command").get<std::string>();
  if (!is_safe_command(command)) {
    throw std::runtime_error(
        "Command blocked. Allowed prefixes: ls, pwd, echo, cat, head, tail, grep, find");
  }

  std::array<char, 256> buffer{};
  std::string output;
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("Failed to execute command");
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    output += buffer.data();
  }

  const int status = pclose(pipe);
  if (status != 0) {
    output += "\n[exit code: " + std::to_string(status) + "]";
  }
  return output.empty() ? "(no output)" : output;
}

}  // namespace

nlohmann::json tool_definitions() {
  return nlohmann::json::array({
      {{"type", "function"},
       {"function",
        {{"name", "get_time"},
         {"description", "Return current local date and time"},
         {"parameters", {{"type", "object"}, {"properties", nlohmann::json::object()}, {"required", nlohmann::json::array()}}}}}},
      {{"type", "function"},
       {"function",
        {{"name", "read_file"},
         {"description", "Read a text file from disk. Use only when the user explicitly asks to open a file."},
         {"parameters",
          {{"type", "object"},
           {"properties", {{"path", {{"type", "string"}, {"description", "Absolute or relative file path"}}}}},
           {"required", nlohmann::json::array({"path"})}}}}}},
      {{"type", "function"},
       {"function",
        {{"name", "list_directory"},
         {"description", "List files in a directory. Use only when the user explicitly asks to inspect a folder."},
         {"parameters",
          {{"type", "object"},
           {"properties", {{"path", {{"type", "string"}, {"description", "Directory path"}}}}},
           {"required", nlohmann::json::array({"path"})}}}}}},
      {{"type", "function"},
       {"function",
        {{"name", "run_command"},
         {"description", "Run a safe read-only shell command"},
         {"parameters",
          {{"type", "object"},
           {"properties",
            {{"command", {{"type", "string"}, {"description", "Shell command with allowed prefix"}}}}},
           {"required", nlohmann::json::array({"command"})}}}}}},
  });
}

std::string execute_tool(const std::string& name, const nlohmann::json& arguments) {
  try {
    if (name == "get_time") {
      return tool_get_time();
    }
    if (name == "read_file") {
      return tool_read_file(arguments);
    }
    if (name == "list_directory") {
      return tool_list_directory(arguments);
    }
    if (name == "run_command") {
      return tool_run_command(arguments);
    }
    return "Unknown tool: " + name;
  } catch (const std::exception& ex) {
    return std::string("Tool error: ") + ex.what();
  }
}

}  // namespace jarvis
