#pragma once

#include "jarvis/config.hpp"
#include "jarvis/database.hpp"
#include "jarvis/ollama.hpp"
#include "jarvis/rag.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace jarvis {

class Agent {
 public:
  Agent(const Config& config, OllamaClient& ollama, RagStore& rag, Database& database);

  std::string ask(const std::string& user_message);
  void reset();
  const std::vector<nlohmann::json>& history() const { return history_; }

 private:
  const Config& config_;
  OllamaClient& ollama_;
  RagStore& rag_;
  Database& database_;
  std::vector<nlohmann::json> history_;
  static constexpr int kMaxToolRounds = 8;

  nlohmann::json build_messages(const std::string& user_message) const;
  static std::string system_prompt();
};

}  // namespace jarvis
