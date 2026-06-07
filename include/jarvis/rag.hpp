#pragma once

#include "jarvis/config.hpp"
#include "jarvis/database.hpp"
#include "jarvis/ollama.hpp"

#include <string>
#include <vector>

namespace jarvis {

struct RagHit {
  std::string source;
  std::string text;
  float score = 0.0f;
};

class RagStore {
 public:
  RagStore(const Config& config, OllamaClient& ollama, Database& database);

  void build_index();
  bool load_index();
  std::vector<RagHit> search(const std::string& query) const;
  std::string build_context(const std::string& query) const;
  std::size_t chunk_count() const;

 private:
  const Config& config_;
  OllamaClient& ollama_;
  Database& database_;
  std::vector<StoredChunk> chunks_;

  void reload_chunks();
};

}  // namespace jarvis
