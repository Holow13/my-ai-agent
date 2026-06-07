#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace jarvis {

class OllamaClient {
 public:
  explicit OllamaClient(std::string host);

  bool is_available() const;
  std::vector<float> embed(const std::string& model, const std::string& text) const;
  nlohmann::json chat(const std::string& model, const nlohmann::json& messages,
                      const nlohmann::json& tools = nlohmann::json::array()) const;

 private:
  std::string host_;
  std::string host_header_;
  int port_ = 11434;
};

}  // namespace jarvis
