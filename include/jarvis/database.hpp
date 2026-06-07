#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace jarvis {

struct ChatRecord {
  std::string role;
  std::string content;
  std::string created_at;
};

struct StoredChunk {
  std::int64_t id = 0;
  std::string source;
  std::string text;
  std::vector<float> embedding;
};

class Database {
 public:
  explicit Database(std::string path);
  ~Database();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void open();
  void migrate();
  void seed_defaults();
  void clear_index();

  std::int64_t upsert_document(const std::string& path, const std::string& title);
  std::int64_t insert_chunk(std::int64_t document_id, int position, const std::string& text);
  void insert_embedding(std::int64_t chunk_id, const std::vector<float>& embedding);

  std::vector<StoredChunk> load_chunks() const;
  std::size_t chunk_count() const;

  void save_chat(const std::string& role, const std::string& content);
  std::vector<ChatRecord> recent_chats(std::size_t limit = 10) const;

  void set_fact(const std::string& key, const std::string& value);
  std::string get_fact(const std::string& key) const;

  bool import_legacy_json(const std::string& json_path);

 private:
  std::string path_;
  sqlite3* db_ = nullptr;

  void exec(const std::string& sql) const;
};

}  // namespace jarvis
