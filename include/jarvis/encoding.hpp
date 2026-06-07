#pragma once

#include <string>

namespace jarvis {

void setup_console_encoding();
bool is_valid_utf8(const std::string& text);
std::string ensure_utf8(const std::string& text);

}  // namespace jarvis
