#include "jarvis/utils.hpp"
#include "jarvis/encoding.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace jarvis {

std::string trim(const std::string& text) {
  const auto begin = std::find_if_not(text.begin(), text.end(),
                                      [](unsigned char ch) { return std::isspace(ch); });
  const auto end = std::find_if_not(text.rbegin(), text.rend(),
                                    [](unsigned char ch) { return std::isspace(ch); })
                       .base();
  if (begin >= end) {
    return "";
  }
  return std::string(begin, end);
}

std::string read_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Cannot read file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void write_file(const std::string& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("Cannot write file: " + path);
  }
  output << content;
}

std::vector<std::string> split_chunks(const std::string& text, std::size_t chunk_size,
                                      std::size_t overlap) {
  std::vector<std::string> chunks;
  if (text.empty() || chunk_size == 0) {
    return chunks;
  }

  const std::size_t step = chunk_size > overlap ? chunk_size - overlap : chunk_size;
  for (std::size_t start = 0; start < text.size(); start += step) {
    while (start < text.size() &&
           (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
      ++start;
    }
    if (start >= text.size()) {
      break;
    }
    const std::size_t end = std::min(start + chunk_size, text.size());
    std::string piece = trim(text.substr(start, end - start));
    piece = fix_utf8_boundaries(piece);
    if (!piece.empty()) {
      chunks.push_back(piece);
    }
    if (end >= text.size()) {
      break;
    }
  }
  return chunks;
}

float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
  if (a.empty() || a.size() != b.size()) {
    return 0.0f;
  }

  double dot = 0.0;
  double norm_a = 0.0;
  double norm_b = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
    norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
  }

  if (norm_a == 0.0 || norm_b == 0.0) {
    return 0.0f;
  }
  return static_cast<float>(dot / (std::sqrt(norm_a) * std::sqrt(norm_b)));
}

std::string to_lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return text;
}

}  // namespace jarvis
