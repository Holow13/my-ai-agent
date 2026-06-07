#include "jarvis/ollama.hpp"

#include <httplib.h>
#include <stdexcept>

namespace jarvis {

namespace {

std::pair<std::string, int> parse_host(const std::string& host) {
  const std::string prefix = "http://";
  std::string value = host;
  if (value.rfind(prefix, 0) == 0) {
    value = value.substr(prefix.size());
  }

  const auto colon = value.find(':');
  if (colon == std::string::npos) {
    return {value, 11434};
  }

  const std::string hostname = value.substr(0, colon);
  const int port = std::stoi(value.substr(colon + 1));
  return {hostname, port};
}

}  // namespace

OllamaClient::OllamaClient(std::string host) : host_(std::move(host)) {
  const auto parsed = parse_host(host_);
  host_header_ = parsed.first;
  port_ = parsed.second;
}

bool OllamaClient::is_available() const {
  httplib::Client client(host_header_, port_);
  client.set_connection_timeout(3, 0);
  client.set_read_timeout(3, 0);
  const auto response = client.Get("/api/tags");
  return response && response->status == 200;
}

std::vector<float> OllamaClient::embed(const std::string& model,
                                       const std::string& text) const {
  httplib::Client client(host_header_, port_);
  client.set_connection_timeout(10, 0);
  client.set_read_timeout(120, 0);

  nlohmann::json payload = {{"model", model}, {"prompt", text}};
  const auto response = client.Post("/api/embeddings", payload.dump(), "application/json");
  if (!response || response->status != 200) {
    const std::string details = response ? response->body : "no response";
    throw std::runtime_error("Ollama embeddings failed: " + details);
  }

  const auto json = nlohmann::json::parse(response->body);
  std::vector<float> embedding;
  for (const auto& value : json.at("embedding")) {
    embedding.push_back(value.get<float>());
  }
  return embedding;
}

nlohmann::json OllamaClient::chat(const std::string& model, const nlohmann::json& messages,
                                  const nlohmann::json& tools) const {
  httplib::Client client(host_header_, port_);
  client.set_connection_timeout(10, 0);
  client.set_read_timeout(300, 0);

  nlohmann::json payload = {{"model", model}, {"messages", messages}, {"stream", false}};
  if (!tools.empty()) {
    payload["tools"] = tools;
  }

  const auto response = client.Post("/api/chat", payload.dump(), "application/json");
  if (!response || response->status != 200) {
    const std::string details = response ? response->body : "no response";
    throw std::runtime_error("Ollama chat failed: " + details);
  }

  return nlohmann::json::parse(response->body);
}

}  // namespace jarvis
