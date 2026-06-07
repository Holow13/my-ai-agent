#pragma once

#include "jarvis/config.hpp"
#include "jarvis/ollama.hpp"

#include <string>
#include <vector>

namespace jarvis {

struct RagChunk {
  std::string source;
  std::string text;
  std::vector<float> embedding;
};

struct RagHit {
  std::string source;
  std::string text;
  float score = 0.0f;
};

class RagStore {
 public:
  RagStore(const Config& config, OllamaClient& ollama);

  void build_index();
  bool load_index();
  void save_index() const;
  std::vector<RagHit> search(const std::string& query) const;
  std::string build_context(const std::string& query) const;
  std::size_t chunk_count() const { return chunks_.size(); }

 private:
  const Config& config_;
  OllamaClient& ollama_;
  std::vector<RagChunk> chunks_;
};

}  // namespace jarvis
