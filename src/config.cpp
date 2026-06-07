#include "jarvis/config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace jarvis {

Config load_config(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Cannot open config file: " + path);
  }

  nlohmann::json json;
  input >> json;

  Config config;
  if (json.contains("ollama_host")) {
    config.ollama_host = json["ollama_host"].get<std::string>();
  }
  if (json.contains("chat_model")) {
    config.chat_model = json["chat_model"].get<std::string>();
  }
  if (json.contains("embed_model")) {
    config.embed_model = json["embed_model"].get<std::string>();
  }
  if (json.contains("documents_dir")) {
    config.documents_dir = json["documents_dir"].get<std::string>();
  }
  if (json.contains("index_dir")) {
    config.index_dir = json["index_dir"].get<std::string>();
  }
  if (json.contains("db_path")) {
    config.db_path = json["db_path"].get<std::string>();
  }
  if (json.contains("rag_top_k")) {
    config.rag_top_k = json["rag_top_k"].get<std::size_t>();
  }
  if (json.contains("chunk_size")) {
    config.chunk_size = json["chunk_size"].get<std::size_t>();
  }
  if (json.contains("chunk_overlap")) {
    config.chunk_overlap = json["chunk_overlap"].get<std::size_t>();
  }
  return config;
}

}  // namespace jarvis
