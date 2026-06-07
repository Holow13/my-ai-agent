#pragma once

#include <string>

namespace jarvis {

void setup_console_encoding();
bool is_valid_utf8(const std::string& text);
std::string ensure_utf8(const std::string& text);
std::string read_text_file(const std::string& path);
std::string fix_utf8_boundaries(const std::string& text);

}  // namespace jarvis
