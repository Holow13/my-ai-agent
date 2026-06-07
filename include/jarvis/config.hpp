#pragma once

#include <cstddef>
#include <string>

namespace jarvis {

struct Config {
  std::string ollama_host = "http://127.0.0.1:11434";
  std::string chat_model = "llama3.2";
  std::string embed_model = "nomic-embed-text";
  std::string documents_dir = "data/documents";
  std::string index_dir = "data/index";
  std::size_t rag_top_k = 4;
  std::size_t chunk_size = 600;
  std::size_t chunk_overlap = 100;
};

Config load_config(const std::string& path);

}  // namespace jarvis
