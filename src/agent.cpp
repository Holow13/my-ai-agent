#include "jarvis/agent.hpp"

#include "jarvis/tools.hpp"
#include "jarvis/database.hpp"
#include "jarvis/utils.hpp"

#include <sstream>
#include <stdexcept>

namespace jarvis {

namespace {

bool contains_any(const std::string& text, const std::initializer_list<const char*> phrases) {
  const std::string lower = to_lower(text);
  for (const char* phrase : phrases) {
    if (lower.find(phrase) != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::string expand_rag_query(const std::string& user_message) {
  if (contains_any(user_message, {"кто я", "обо мне", "кто такой", "а я кто", "я кто",
                                    "что ты знаешь", "мои проект", "бро", "братан",
                                    "who am i", "about me"})) {
    return user_message + " about_me обо мне пользователь имя город учеба projects";
  }
  return user_message;
}

bool should_use_tools(const std::string& user_message) {
  if (contains_any(user_message, {"кто я", "обо мне", "кто такой", "а я кто", "я кто",
                                  "что ты знаешь", "мои проект", "что ты умеешь",
                                  "бро", "братан", "who am i", "about me"})) {
    return false;
  }

  return contains_any(user_message,
                    {"время", "дата", "файл", "прочитай", "открой", "папк", "директор", "каталог",
                     "list", "ls", "dir", "команда", "shell", "status", "статус системы"});
}

}  // namespace

Agent::Agent(const Config& config, OllamaClient& ollama, RagStore& rag, Database& database)
    : config_(config), ollama_(ollama), rag_(rag), database_(database) {}

void Agent::reset() { history_.clear(); }

std::string Agent::system_prompt() {
  return R"(You are JARVIS, a concise personal AI assistant.
Answer questions about the user, their identity, projects, and preferences using ONLY the knowledge base context in this system message.
Do not invent files, folders, or facts that are not in the knowledge base or tool results.
Use tools only when the user explicitly asks for time, filesystem access, or shell inspection.
Prefer short, actionable answers in Russian unless the user writes in another language.
If the knowledge base contains the answer, respond directly and do not call tools.
Words like "бро" or "братан" are informal greetings, not questions about siblings.)";
}

nlohmann::json Agent::build_messages(const std::string& user_message) const {
  std::ostringstream system;
  system << system_prompt();

  const std::string rag_context = rag_.build_context(expand_rag_query(user_message));
  if (!rag_context.empty()) {
    system << "\n\n" << rag_context;
  }

  nlohmann::json messages = nlohmann::json::array();
  messages.push_back({{"role", "system"}, {"content", system.str()}});
  for (const auto& message : history_) {
    messages.push_back(message);
  }
  messages.push_back({{"role", "user"}, {"content", user_message}});
  return messages;
}

std::string Agent::ask(const std::string& user_message) {
  nlohmann::json messages = build_messages(user_message);
  const auto tools = should_use_tools(user_message) ? tool_definitions() : nlohmann::json::array();

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
    database_.save_chat("user", user_message);
    database_.save_chat("assistant", content);
    return content;
  }

  throw std::runtime_error("Tool loop exceeded maximum rounds");
}

}  // namespace jarvis
