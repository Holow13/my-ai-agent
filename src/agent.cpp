#include "jarvis/agent.hpp"

#include "jarvis/tools.hpp"

#include <sstream>
#include <stdexcept>

namespace jarvis {

Agent::Agent(const Config& config, OllamaClient& ollama, RagStore& rag)
    : config_(config), ollama_(ollama), rag_(rag) {}

void Agent::reset() { history_.clear(); }

std::string Agent::system_prompt() {
  return R"(You are JARVIS, a concise personal AI assistant.
Use the provided knowledge base context when it is relevant.
Use tools when you need current time, filesystem access, or shell inspection.
Prefer short, actionable answers in Russian unless the user writes in another language.
If a tool fails, explain the issue briefly and continue with best-effort reasoning.)";
}

nlohmann::json Agent::build_messages(const std::string& user_message) const {
  std::ostringstream enriched;
  enriched << user_message;

  const std::string rag_context = rag_.build_context(user_message);
  if (!rag_context.empty()) {
    enriched << "\n\n" << rag_context;
  }

  nlohmann::json messages = nlohmann::json::array();
  messages.push_back({{"role", "system"}, {"content", system_prompt()}});
  for (const auto& message : history_) {
    messages.push_back(message);
  }
  messages.push_back({{"role", "user"}, {"content", enriched.str()}});
  return messages;
}

std::string Agent::ask(const std::string& user_message) {
  nlohmann::json messages = build_messages(user_message);
  const auto tools = tool_definitions();

  for (int round = 0; round < kMaxToolRounds; ++round) {
    const auto response = ollama_.chat(config_.chat_model, messages, tools);
    const auto& message = response.at("message");

    if (message.contains("tool_calls") && message.at("tool_calls").is_array() &&
        !message.at("tool_calls").empty()) {
      messages.push_back(message);

      for (const auto& tool_call : message.at("tool_calls")) {
        const auto& function = tool_call.at("function");
        const std::string name = function.at("name").get<std::string>();
        nlohmann::json arguments = nlohmann::json::object();
        if (function.contains("arguments")) {
          if (function.at("arguments").is_string()) {
            arguments = nlohmann::json::parse(function.at("arguments").get<std::string>());
          } else {
            arguments = function.at("arguments");
          }
        }

        const std::string result = execute_tool(name, arguments);
        messages.push_back({{"role", "tool"}, {"content", result}});
      }
      continue;
    }

    const std::string content = message.value("content", "");
    if (content.empty()) {
      throw std::runtime_error("Model returned an empty response");
    }

    history_.push_back({{"role", "user"}, {"content", user_message}});
    history_.push_back({{"role", "assistant"}, {"content", content}});
    return content;
  }

  throw std::runtime_error("Tool loop exceeded maximum rounds");
}

}  // namespace jarvis
