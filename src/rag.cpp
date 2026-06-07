#include "jarvis/rag.hpp"

#include "jarvis/utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
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

RagStore::RagStore(const Config& config, OllamaClient& ollama)
    : config_(config), ollama_(ollama) {}

void RagStore::build_index() {
  chunks_.clear();

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
    const std::string text = read_file(source);
    const auto parts = split_chunks(text, config_.chunk_size, config_.chunk_overlap);

    std::cout << "Indexing " << source << " (" << parts.size() << " chunks)\n";
    for (const auto& part : parts) {
      RagChunk chunk;
      chunk.source = source;
      chunk.text = part;
      chunk.embedding = ollama_.embed(config_.embed_model, part);
      chunks_.push_back(std::move(chunk));
    }
  }

  save_index();
}

bool RagStore::load_index() {
  const fs::path index_path = fs::path(config_.index_dir) / "chunks.json";
  if (!fs::exists(index_path)) {
    return false;
  }

  const auto json = nlohmann::json::parse(read_file(index_path.string()));
  chunks_.clear();
  for (const auto& item : json.at("chunks")) {
    RagChunk chunk;
    chunk.source = item.at("source").get<std::string>();
    chunk.text = item.at("text").get<std::string>();
    for (const auto& value : item.at("embedding")) {
      chunk.embedding.push_back(value.get<float>());
    }
    chunks_.push_back(std::move(chunk));
  }
  return true;
}

void RagStore::save_index() const {
  fs::create_directories(config_.index_dir);

  nlohmann::json json;
  json["chunks"] = nlohmann::json::array();
  for (const auto& chunk : chunks_) {
    nlohmann::json item = {{"source", chunk.source}, {"text", chunk.text}, {"embedding", chunk.embedding}};
    json["chunks"].push_back(std::move(item));
  }

  const fs::path index_path = fs::path(config_.index_dir) / "chunks.json";
  write_file(index_path.string(), json.dump());
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
