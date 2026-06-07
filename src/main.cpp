#include "jarvis/agent.hpp"
#include "jarvis/config.hpp"
#include "jarvis/database.hpp"
#include "jarvis/encoding.hpp"
#include "jarvis/ollama.hpp"
#include "jarvis/rag.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void print_usage() {
  std::cout << "JARVIS personal AI agent\n\n"
            << "Usage:\n"
            << "  jarvis                 Interactive chat\n"
            << "  jarvis index           Build RAG index from data/documents\n"
            << "  jarvis ask <message>   Single question\n"
            << "  jarvis status          Check Ollama and index status\n"
            << "  jarvis history         Show recent chat messages\n"
            << "  jarvis facts           Show user profile facts\n";
}

std::string join_args(int argc, char** argv, int start_index) {
  std::ostringstream joined;
  for (int i = start_index; i < argc; ++i) {
    if (i > start_index) {
      joined << ' ';
    }
    joined << argv[i];
  }
  return joined.str();
}

}  // namespace

int main(int argc, char** argv) {
  try {
    jarvis::setup_console_encoding();
    const std::string config_path = "config.json";
    const jarvis::Config config = jarvis::load_config(config_path);

    jarvis::Database database(config.db_path);
    database.open();
    database.migrate();
    database.seed_defaults();

    jarvis::OllamaClient ollama(config.ollama_host);
    jarvis::RagStore rag(config, ollama, database);
    jarvis::Agent agent(config, ollama, rag, database);

    const std::string command = argc > 1 ? argv[1] : "chat";

    if (command == "status") {
      std::cout << "Ollama: " << (ollama.is_available() ? "online" : "offline") << '\n';
      std::cout << "Database: " << config.db_path << " (" << database.chunk_count() << " chunks)\n";
      if (rag.load_index()) {
        std::cout << "RAG index: loaded (" << rag.chunk_count() << " chunks)\n";
      } else {
        std::cout << "RAG index: missing (run `jarvis index`)\n";
      }
      return 0;
    }

    if (command == "history") {
      const auto records = database.recent_chats(10);
      if (records.empty()) {
        std::cout << "No chat history yet.\n";
        return 0;
      }
      for (const auto& record : records) {
        std::cout << "[" << record.created_at << "] " << record.role << "> " << record.content << "\n\n";
      }
      return 0;
    }

    if (command == "facts") {
      const auto keys = {"name", "city", "university", "direction", "course"};
      for (const char* key : keys) {
        std::cout << key << ": " << database.get_fact(key) << '\n';
      }
      return 0;
    }

    if (command == "index") {
      if (!ollama.is_available()) {
        std::cerr << "Ollama is not available at " << config.ollama_host << '\n';
        return 1;
      }
      rag.build_index();
      std::cout << "Index built: " << rag.chunk_count() << " chunks\n";
      std::cout << "Saved to: " << config.db_path << '\n';
      return 0;
    }

    if (!ollama.is_available()) {
      std::cerr << "Ollama is not available. Start it with: ollama serve\n";
      std::cerr << "Then pull models:\n"
                << "  ollama pull " << config.chat_model << '\n'
                << "  ollama pull " << config.embed_model << '\n';
      return 1;
    }

    if (!rag.load_index()) {
      std::cout << "RAG index not found. Building from documents...\n";
      rag.build_index();
    }

    if (command == "ask") {
      if (argc < 3) {
        print_usage();
        return 1;
      }
      const std::string answer = agent.ask(jarvis::ensure_utf8(join_args(argc, argv, 2)));
      std::cout << answer << '\n';
      return 0;
    }

    if (command == "chat" || command == "jarvis") {
      std::cout << "JARVIS ready. Type 'exit' to quit.\n";
      std::string line;
      while (true) {
        std::cout << "\nYou> ";
        if (!std::getline(std::cin, line)) {
          break;
        }
        if (line == "exit" || line == "quit") {
          break;
        }
        if (line.empty()) {
          continue;
        }

        try {
          const std::string answer = agent.ask(jarvis::ensure_utf8(line));
          std::cout << "JARVIS> " << answer << '\n';
        } catch (const std::exception& ex) {
          std::cerr << "Error: " << ex.what() << '\n';
        }
      }
      return 0;
    }

    print_usage();
    return 1;
  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << '\n';
    return 1;
  }
}
