#pragma once

#include <string>
#include <vector>

namespace jarvis {

std::string trim(const std::string& text);
std::string read_file(const std::string& path);
void write_file(const std::string& path, const std::string& content);
std::vector<std::string> split_chunks(const std::string& text, std::size_t chunk_size,
                                      std::size_t overlap);
float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);
std::string to_lower(std::string text);

}  // namespace jarvis
