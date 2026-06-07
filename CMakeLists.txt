cmake_minimum_required(VERSION 3.16)
project(jarvis_agent VERSION 0.2.0 LANGUAGES C CXX)

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
FetchContent_Declare(
  sqlite3_amalgamation
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
)
FetchContent_MakeAvailable(json httplib)
FetchContent_GetProperties(sqlite3_amalgamation)
if(NOT sqlite3_amalgamation_POPULATED)
  FetchContent_Populate(sqlite3_amalgamation)
endif()

add_library(sqlite3 STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)

add_executable(jarvis
  src/main.cpp
  src/config.cpp
  src/database.cpp
  src/ollama.cpp
  src/rag.cpp
  src/tools.cpp
  src/agent.cpp
  src/utils.cpp
  src/encoding.cpp
)

target_include_directories(jarvis PRIVATE include)
target_link_libraries(jarvis PRIVATE nlohmann_json::nlohmann_json httplib::httplib sqlite3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(jarvis PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS jarvis RUNTIME DESTINATION bin)
install(FILES config.json DESTINATION share/jarvis)
