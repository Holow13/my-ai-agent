#include "jarvis/rag.hpp"

#include "jarvis/encoding.hpp"
#include "jarvis/utils.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace jarvis {

namespace {

bool is_text_document(const fs::path& path) {
  const auto ext = to_lower(path.extension().string());
  return ext == ".md" || ext == ".txt" || ext == ".markdown";
}

}  // namespace

RagStore::RagStore(const Config& config, OllamaClient& ollama, Database& database)
    : config_(config), ollama_(ollama), database_(database) {}

void RagStore::reload_chunks() { chunks_ = database_.load_chunks(); }

std::size_t RagStore::chunk_count() const { return chunks_.size(); }

void RagStore::build_index() {
  chunks_.clear();
  database_.clear_index();

  if (!fs::exists(config_.documents_dir)) {
    fs::create_directories(config_.documents_dir);
    std::cout << "Created documents directory: " << config_.documents_dir << '\n';
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(config_.documents_dir)) {
    if (!entry.is_regular_file() || !is_text_document(entry.path())) {
      continue;
    }

    const std::string source = entry.path().string();
    const std::string text = read_text_file(source);
    const auto parts = split_chunks(text, config_.chunk_size, config_.chunk_overlap);
    const std::int64_t document_id =
        database_.upsert_document(source, entry.path().filename().string());

    std::cout << "Indexing " << source << " (" << parts.size() << " chunks)\n";
    for (std::size_t i = 0; i < parts.size(); ++i) {
      const std::int64_t chunk_id =
          database_.insert_chunk(document_id, static_cast<int>(i), parts[i]);
      const auto embedding = ollama_.embed(config_.embed_model, parts[i]);
      database_.insert_embedding(chunk_id, embedding);
    }
  }

  reload_chunks();
}

bool RagStore::load_index() {
  const fs::path legacy_index = fs::path(config_.index_dir) / "chunks.json";
  if (database_.chunk_count() == 0 && fs::exists(legacy_index)) {
    std::cout << "Migrating legacy JSON index to SQLite...\n";
    if (database_.import_legacy_json(legacy_index.string())) {
      reload_chunks();
      return !chunks_.empty();
    }
  }

  reload_chunks();
  return !chunks_.empty();
}

std::vector<RagHit> RagStore::search(const std::string& query) const {
  if (chunks_.empty()) {
    return {};
  }

  const auto query_embedding = ollama_.embed(config_.embed_model, query);
  std::vector<RagHit> hits;
  hits.reserve(chunks_.size());

  for (const auto& chunk : chunks_) {
    RagHit hit;
    hit.source = chunk.source;
    hit.text = chunk.text;
    hit.score = cosine_similarity(query_embedding, chunk.embedding);
    hits.push_back(hit);
  }

  std::sort(hits.begin(), hits.end(),
            [](const RagHit& left, const RagHit& right) { return left.score > right.score; });

  if (hits.size() > config_.rag_top_k) {
    hits.resize(config_.rag_top_k);
  }
  return hits;
}

std::string RagStore::build_context(const std::string& query) const {
  const auto hits = search(query);
  if (hits.empty()) {
    return "";
  }

  std::string context = "Relevant knowledge base excerpts:\n";
  for (std::size_t i = 0; i < hits.size(); ++i) {
    context += "\n[" + std::to_string(i + 1) + "] source: " + hits[i].source + "\n";
    context += hits[i].text + "\n";
  }
  return context;
}

}  // namespace jarvis
