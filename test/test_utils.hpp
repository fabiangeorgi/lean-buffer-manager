#pragma once
//
// #include <filesystem>
// #include <fstream>
//
#include "buffer_frame.hpp"
#include "buffer_manager.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

// Custom Matcher to compare to byte arrays
MATCHER_P2(MatchesBytes, expected, length, "bytes match") {
  static_assert(sizeof(*arg) == 1, "Only support comparison of char* or byte*");
  std::string_view actual_view{reinterpret_cast<char*>(arg), static_cast<size_t>(length)};
  std::string_view expected_view{reinterpret_cast<char*>(expected), static_cast<size_t>(length)};

  if (actual_view != expected_view) {
    *result_listener << "\n     Got: ";
    if (length <= 100) {
      *result_listener << actual_view << "'\nbut it does not match: '" << expected_view << "'";
    } else {
      *result_listener << "[values too large to print]";
    }
    return false;
  }
  return true;
}

// Generate a random data with given `length`.
std::vector<std::byte> generate_random_data(size_t length);

// Generate a Page filled with random data.
Page generate_random_page();

// Read all content of a file into a string.
std::string read_file(const std::filesystem::path& path);

// Print `len` bytes starting from `start` (as integers, i.e., "66 42 64" and not as "a b c").
void print_bytes(std::byte* start, size_t len);

// Print `len` bytes starting from `start` (as integers, i.e., "66 42 64" and not as "a b c").
void print_bytes(char* start, size_t len);

std::string get_user();

// Writes the given value in the frame's page data.
void store_u64(BufferFrame* frame, uint64_t value);

// Reads the given frame's page data and returns it as uint64_t.
uint64_t get_u64(BufferFrame* frame);

// Base class for all tests that need a BufferManager.
class BaseTest : public ::testing::Test {
 protected:
  // Set all member variables for default creation.
  virtual void InitVars() = 0;

  void SetUp() override;
  void TearDown() override;

  std::unique_ptr<BufferManager> create_default_bm();

 protected:
  size_t _frame_count;
  size_t _page_count;

  // Base directories in which to create SSD files.
  std::filesystem::path _base_dir_ssd;

  // Actual file paths.
  std::filesystem::path _ssd_path;
  std::filesystem::path _initial_ssd_data_path;
};
