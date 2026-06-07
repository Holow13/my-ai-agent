#include "jarvis/encoding.hpp"

#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace jarvis {

bool is_valid_utf8(const std::string& text) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
  std::size_t i = 0;

  while (i < text.size()) {
    const unsigned char byte = bytes[i];
    if (byte <= 0x7F) {
      ++i;
      continue;
    }

    std::size_t length = 0;
    if ((byte & 0xE0) == 0xC0) {
      length = 2;
    } else if ((byte & 0xF0) == 0xE0) {
      length = 3;
    } else if ((byte & 0xF8) == 0xF0) {
      length = 4;
    } else {
      return false;
    }

    if (i + length > text.size()) {
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

namespace {

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

std::string ensure_utf8(const std::string& text) {
  if (text.empty() || is_valid_utf8(text)) {
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

  throw std::runtime_error("Input is not valid UTF-8");
}

}  // namespace jarvis
