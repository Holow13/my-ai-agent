cmake_minimum_required(VERSION 3.16)
project(jarvis_agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
)
FetchContent_MakeAvailable(json httplib)

add_executable(jarvis
  src/main.cpp
  src/config.cpp
  src/ollama.cpp
  src/rag.cpp
  src/tools.cpp
  src/agent.cpp
  src/utils.cpp
  src/encoding.cpp
)

target_include_directories(jarvis PRIVATE include)
target_link_libraries(jarvis PRIVATE nlohmann_json::nlohmann_json httplib::httplib)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(jarvis PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS jarvis RUNTIME DESTINATION bin)
install(FILES config.json DESTINATION share/jarvis)
