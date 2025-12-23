cmake_minimum_required(VERSION 3.20)

project(jawiki_louds_cpp
  VERSION 0.1.0
  LANGUAGES CXX
)

# -----------------------------
# Global settings
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Library: core sources
# -----------------------------
add_library(core STATIC
  # prefix (existing)
  src/prefix/prefix_tree.cpp

  # louds (existing)
  src/louds/converter.cpp
  src/louds/louds.cpp

  # termId prefix (existing char32)
  src/prefix_with_term_id/prefix_tree_with_term_id.cpp

  # termId louds (existing char32)
  src/louds_with_term_id/converter_with_term_id.cpp
  src/louds_with_term_id/louds_with_term_id.cpp

  # -----------------------------
  # NEW: UTF-16 with_term_id
  # -----------------------------
  src/prefix_with_term_id/prefix_tree_with_term_id_utf16.cpp
  src/louds_with_term_id/converter_with_term_id_utf16.cpp
  src/louds_with_term_id/louds_with_term_id_utf16_writer.cpp
  src/louds_with_term_id/louds_with_term_id_utf16_reader.cpp
)

target_include_directories(core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(core PUBLIC cxx_std_20)

# -----------------------------
# zlib (for jawiki_build .gz reader)
# -----------------------------
find_package(ZLIB REQUIRED)

# -----------------------------
# Tool: jawiki_build (CLI)
# -----------------------------
add_executable(jawiki_build
  src/tools/jawiki_build.cpp
)

target_link_libraries(jawiki_build PRIVATE core ZLIB::ZLIB)
target_compile_features(jawiki_build PRIVATE cxx_std_20)

# -----------------------------
# NEW Tool: jawiki_build_utf16 (CLI)
# -----------------------------
add_executable(jawiki_build_utf16
  src/tools/jawiki_build_utf16.cpp
)

target_link_libraries(jawiki_build_utf16 PRIVATE core ZLIB::ZLIB)
target_compile_features(jawiki_build_utf16 PRIVATE cxx_std_20)

# -----------------------------
# Tool: louds_query (load .bin and query)
# -----------------------------
add_executable(louds_query
  src/tools/louds_query.cpp
)

target_link_libraries(louds_query PRIVATE core)
target_compile_features(louds_query PRIVATE cxx_std_20)

# -----------------------------
# NEW Tool: louds_query_utf16 (load .bin and query)
# -----------------------------
add_executable(louds_query_utf16
  src/tools/louds_query_utf16.cpp
)

target_link_libraries(louds_query_utf16 PRIVATE core)
target_compile_features(louds_query_utf16 PRIVATE cxx_std_20)

# -----------------------------
# Tests
# -----------------------------
enable_testing()

add_executable(test_louds
  tests/test_louds.cpp
)
target_link_libraries(test_louds PRIVATE core)
add_test(NAME test_louds COMMAND test_louds)

add_executable(test_louds_with_term_id
  tests/test_louds_with_term_id.cpp
)
target_link_libraries(test_louds_with_term_id PRIVATE core)
add_test(NAME test_louds_with_term_id COMMAND test_louds_with_term_id)

# -----------------------------
# NEW: UTF-16 with_term_id tests
# -----------------------------
add_executable(test_louds_with_term_id_utf16_writer
  tests/test_louds_with_term_id_utf16_writer.cpp
)
target_link_libraries(test_louds_with_term_id_utf16_writer PRIVATE core)
add_test(NAME test_louds_with_term_id_utf16_writer COMMAND test_louds_with_term_id_utf16_writer)

add_executable(test_louds_with_term_id_utf16_reader
  tests/test_louds_with_term_id_utf16_reader.cpp
)
target_link_libraries(test_louds_with_term_id_utf16_reader PRIVATE core)
add_test(NAME test_louds_with_term_id_utf16_reader COMMAND test_louds_with_term_id_utf16_reader)
