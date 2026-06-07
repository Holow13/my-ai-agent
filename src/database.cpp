#include "jarvis/database.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <sqlite3.h>

namespace fs = std::filesystem;

namespace jarvis {

namespace {

std::string now_iso8601() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm{};
#if defined(_WIN32)
  localtime_s(&local_tm, &time);
#else
  localtime_r(&time, &local_tm);
#endif
  std::ostringstream stream;
  stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return stream.str();
}

std::vector<float> blob_to_vector(const void* data, int bytes) {
  if (data == nullptr || bytes <= 0 || bytes % static_cast<int>(sizeof(float)) != 0) {
    return {};
  }
  const auto* floats = static_cast<const float*>(data);
  const std::size_t count = static_cast<std::size_t>(bytes) / sizeof(float);
  return std::vector<float>(floats, floats + count);
}

}  // namespace

Database::Database(std::string path) : path_(std::move(path)) {}

Database::~Database() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void Database::open() {
  if (db_ != nullptr) {
    return;
  }

  const fs::path db_path(path_);
  if (db_path.has_parent_path()) {
    fs::create_directories(db_path.parent_path());
  }

  if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
    throw std::runtime_error("Cannot open database: " + path_);
  }
}

void Database::exec(const std::string& sql) const {
  char* error_message = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_message) != SQLITE_OK) {
    std::string message = error_message != nullptr ? error_message : "unknown sqlite error";
    sqlite3_free(error_message);
    throw std::runtime_error("SQLite error: " + message);
  }
}

void Database::migrate() {
  exec(R"(
    PRAGMA foreign_keys = ON;

    CREATE TABLE IF NOT EXISTS documents (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      path TEXT NOT NULL UNIQUE,
      title TEXT,
      indexed_at TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS chunks (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      document_id INTEGER NOT NULL,
      position INTEGER NOT NULL,
      text TEXT NOT NULL,
      FOREIGN KEY(document_id) REFERENCES documents(id) ON DELETE CASCADE
    );

    CREATE TABLE IF NOT EXISTS embeddings (
      chunk_id INTEGER PRIMARY KEY,
      vector BLOB NOT NULL,
      FOREIGN KEY(chunk_id) REFERENCES chunks(id) ON DELETE CASCADE
    );

    CREATE TABLE IF NOT EXISTS chat_history (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      role TEXT NOT NULL,
      content TEXT NOT NULL,
      created_at TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS user_facts (
      key TEXT PRIMARY KEY,
      value TEXT NOT NULL
    );
  )");
}

void Database::seed_defaults() {
  const auto seed = [&](const std::string& key, const std::string& value) {
    if (get_fact(key).empty()) {
      set_fact(key, value);
    }
  };

  seed("name", "Максим");
  seed("city", "Новосибирск");
  seed("university", "СибГУ");
  seed("direction", "ИиВТ");
  seed("course", "2");
}

void Database::clear_index() {
  exec("DELETE FROM embeddings;");
  exec("DELETE FROM chunks;");
  exec("DELETE FROM documents;");
}

std::int64_t Database::upsert_document(const std::string& path, const std::string& title) {
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "INSERT INTO documents(path, title, indexed_at) VALUES(?, ?, ?) "
      "ON CONFLICT(path) DO UPDATE SET title = excluded.title, indexed_at = excluded.indexed_at "
      "RETURNING id;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare document upsert");
  }

  const std::string indexed_at = now_iso8601();
  sqlite3_bind_text(statement, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 2, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 3, indexed_at.c_str(), -1, SQLITE_TRANSIENT);

  std::int64_t document_id = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    document_id = sqlite3_column_int64(statement, 0);
  } else {
    sqlite3_finalize(statement);
    throw std::runtime_error("Failed to upsert document");
  }

  sqlite3_finalize(statement);
  return document_id;
}

std::int64_t Database::insert_chunk(std::int64_t document_id, int position, const std::string& text) {
  sqlite3_stmt* statement = nullptr;
  const char* sql = "INSERT INTO chunks(document_id, position, text) VALUES(?, ?, ?);";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare chunk insert");
  }

  sqlite3_bind_int64(statement, 1, document_id);
  sqlite3_bind_int(statement, 2, position);
  sqlite3_bind_text(statement, 3, text.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw std::runtime_error("Failed to insert chunk");
  }

  const std::int64_t chunk_id = sqlite3_last_insert_rowid(db_);
  sqlite3_finalize(statement);
  return chunk_id;
}

void Database::insert_embedding(std::int64_t chunk_id, const std::vector<float>& embedding) {
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "INSERT INTO embeddings(chunk_id, vector) VALUES(?, ?) "
      "ON CONFLICT(chunk_id) DO UPDATE SET vector = excluded.vector;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare embedding insert");
  }

  sqlite3_bind_int64(statement, 1, chunk_id);
  sqlite3_bind_blob(statement, 2, embedding.data(),
                    static_cast<int>(embedding.size() * sizeof(float)), SQLITE_TRANSIENT);

  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw std::runtime_error("Failed to insert embedding");
  }

  sqlite3_finalize(statement);
}

std::vector<StoredChunk> Database::load_chunks() const {
  std::vector<StoredChunk> chunks;
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "SELECT c.id, d.path, c.text, e.vector "
      "FROM chunks c "
      "JOIN documents d ON d.id = c.document_id "
      "JOIN embeddings e ON e.chunk_id = c.id "
      "ORDER BY c.id;";

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare chunk load");
  }

  while (sqlite3_step(statement) == SQLITE_ROW) {
    StoredChunk chunk;
    chunk.id = sqlite3_column_int64(statement, 0);
    chunk.source = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    chunk.text = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    chunk.embedding = blob_to_vector(sqlite3_column_blob(statement, 3), sqlite3_column_bytes(statement, 3));
    chunks.push_back(std::move(chunk));
  }

  sqlite3_finalize(statement);
  return chunks;
}

std::size_t Database::chunk_count() const {
  sqlite3_stmt* statement = nullptr;
  const char* sql = "SELECT COUNT(*) FROM chunks;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    return 0;
  }

  std::size_t count = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    count = static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
  }
  sqlite3_finalize(statement);
  return count;
}

void Database::save_chat(const std::string& role, const std::string& content) {
  sqlite3_stmt* statement = nullptr;
  const char* sql = "INSERT INTO chat_history(role, content, created_at) VALUES(?, ?, ?);";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare chat insert");
  }

  const std::string created_at = now_iso8601();
  sqlite3_bind_text(statement, 1, role.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 2, content.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 3, created_at.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw std::runtime_error("Failed to save chat");
  }

  sqlite3_finalize(statement);
}

std::vector<ChatRecord> Database::recent_chats(std::size_t limit) const {
  std::vector<ChatRecord> records;
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "SELECT role, content, created_at FROM chat_history ORDER BY id DESC LIMIT ?;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    return records;
  }

  sqlite3_bind_int64(statement, 1, static_cast<std::int64_t>(limit));
  while (sqlite3_step(statement) == SQLITE_ROW) {
    ChatRecord record;
    record.role = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
    record.content = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    records.push_back(std::move(record));
  }

  sqlite3_finalize(statement);
  std::reverse(records.begin(), records.end());
  return records;
}

void Database::set_fact(const std::string& key, const std::string& value) {
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "INSERT INTO user_facts(key, value) VALUES(?, ?) "
      "ON CONFLICT(key) DO UPDATE SET value = excluded.value;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare fact upsert");
  }

  sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 2, value.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw std::runtime_error("Failed to save fact");
  }
  sqlite3_finalize(statement);
}

std::string Database::get_fact(const std::string& key) const {
  sqlite3_stmt* statement = nullptr;
  const char* sql = "SELECT value FROM user_facts WHERE key = ? LIMIT 1;";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    return {};
  }

  sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  std::string value;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    value = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
  }
  sqlite3_finalize(statement);
  return value;
}

bool Database::import_legacy_json(const std::string& json_path) {
  if (!fs::exists(json_path)) {
    return false;
  }

  std::ifstream input(json_path);
  if (!input) {
    return false;
  }

  const auto json = nlohmann::json::parse(input);
  if (!json.contains("chunks")) {
    return false;
  }

  clear_index();
  for (const auto& item : json.at("chunks")) {
    const std::string source = item.at("source").get<std::string>();
    const std::string text = item.at("text").get<std::string>();
    const fs::path title_path(source);
    const std::int64_t document_id = upsert_document(source, title_path.filename().string());
    const std::int64_t chunk_id = insert_chunk(document_id, 0, text);

    std::vector<float> embedding;
    for (const auto& value : item.at("embedding")) {
      embedding.push_back(value.get<float>());
    }
    insert_embedding(chunk_id, embedding);
  }

  return true;
}

}  // namespace jarvis
