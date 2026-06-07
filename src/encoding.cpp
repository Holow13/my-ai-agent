#include "jarvis/encoding.hpp"

#include "jarvis/utils.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace jarvis {

namespace {

std::size_t utf8_char_length(unsigned char byte) {
  if (byte <= 0x7F) {
    return 1;
  }
  if ((byte & 0xE0) == 0xC0) {
    return 2;
  }
  if ((byte & 0xF0) == 0xE0) {
    return 3;
  }
  if ((byte & 0xF8) == 0xF0) {
    return 4;
  }
  return 0;
}

#ifdef _WIN32
std::string windows_code_page_to_utf8(const std::string& text, unsigned int code_page) {
  if (text.empty()) {
    return text;
  }

  const int wide_size = MultiByteToWideChar(static_cast<UINT>(code_page), 0, text.c_str(),
                                            static_cast<int>(text.size()), nullptr, 0);
  if (wide_size <= 0) {
    return {};
  }

  std::wstring wide(static_cast<std::size_t>(wide_size), L'\0');
  MultiByteToWideChar(static_cast<UINT>(code_page), 0, text.c_str(), static_cast<int>(text.size()),
                      wide.data(), wide_size);

  const int utf8_size =
      WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_size, nullptr, 0, nullptr, nullptr);
  if (utf8_size <= 0) {
    return {};
  }

  std::string utf8(static_cast<std::size_t>(utf8_size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_size, utf8.data(), utf8_size, nullptr,
                      nullptr);
  return utf8;
}
#endif

}  // namespace

bool is_valid_utf8(const std::string& text) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
  std::size_t i = 0;

  while (i < text.size()) {
    const unsigned char byte = bytes[i];
    const std::size_t length = utf8_char_length(byte);
    if (length == 0 || i + length > text.size()) {
      return false;
    }

    for (std::size_t j = 1; j < length; ++j) {
      if ((bytes[i + j] & 0xC0) != 0x80) {
        return false;
      }
    }

    i += length;
  }

  return true;
}

void setup_console_encoding() {
#ifdef _WIN32
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
#endif
}

std::string fix_utf8_boundaries(const std::string& text) {
  if (text.empty() || is_valid_utf8(text)) {
    return text;
  }

  std::string fixed = text;
  while (!fixed.empty() && !is_valid_utf8(fixed)) {
    fixed.pop_back();
  }
  return fixed;
}

std::string ensure_utf8(const std::string& text) {
  if (text.empty()) {
    return text;
  }

  if (is_valid_utf8(text)) {
    return text;
  }

#ifdef _WIN32
  const unsigned int code_pages[] = {1251u, 866u, static_cast<unsigned int>(CP_ACP)};
  for (unsigned int code_page : code_pages) {
    const std::string converted = windows_code_page_to_utf8(text, code_page);
    if (!converted.empty() && is_valid_utf8(converted)) {
      return converted;
    }
  }
#endif

  const std::string fixed = fix_utf8_boundaries(text);
  if (!fixed.empty() && is_valid_utf8(fixed)) {
    return fixed;
  }

  throw std::runtime_error("Input is not valid UTF-8");
}

std::string read_text_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Cannot read file: " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  std::string raw = buffer.str();

  if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
      static_cast<unsigned char>(raw[1]) == 0xBB && static_cast<unsigned char>(raw[2]) == 0xBF) {
    raw.erase(0, 3);
  }

#ifdef _WIN32
  if (raw.size() >= 2) {
    const unsigned char b0 = static_cast<unsigned char>(raw[0]);
    const unsigned char b1 = static_cast<unsigned char>(raw[1]);
    if (b0 == 0xFF && b1 == 0xFE) {
      const int utf8_size = WideCharToMultiByte(
          CP_UTF8, 0, reinterpret_cast<const wchar_t*>(raw.data() + 2),
          static_cast<int>((raw.size() - 2) / sizeof(wchar_t)), nullptr, 0, nullptr, nullptr);
      std::string utf8(static_cast<std::size_t>(utf8_size), '\0');
      WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(raw.data() + 2),
                          static_cast<int>((raw.size() - 2) / sizeof(wchar_t)), utf8.data(),
                          utf8_size, nullptr, nullptr);
      return utf8;
    }
  }
#endif

  return ensure_utf8(raw);
}

}  // namespace jarvis
