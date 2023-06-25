#include "test_utils.hpp"

#include <unistd.h>

#include <cstring>
#include <fstream>
#include <random>

std::vector<std::byte> generate_random_data(size_t length) {
  const auto chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const size_t max_index = strlen(chars) - 1;

  std::mt19937 rng{std::random_device{}()};
  auto randchar = [&] { return static_cast<std::byte>(chars[rng() % max_index]); };

  std::vector<std::byte> data(length);
  std::generate_n(data.begin(), length, randchar);
  return data;
}

Page generate_random_page() {
  Page page{};
  std::vector<std::byte> random_data = generate_random_data(EFFECTIVE_PAGE_SIZE);
  memcpy(page.data(), random_data.data(), EFFECTIVE_PAGE_SIZE);
  return page;
}

std::string read_file(const std::filesystem::path& path) {
  std::stringstream buffer;
  std::ifstream file_stream{path, std::ios_base::binary};
  buffer << file_stream.rdbuf();
  return buffer.str();
}

void print_bytes(std::byte* start, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    std::cout << static_cast<uint64_t>(start[i]) << ' ';
  }
  std::cout << std::endl;
}

void print_bytes(char* start, size_t len) { return print_bytes(reinterpret_cast<std::byte*>(start), len); }

void store_u64(BufferFrame* frame, uint64_t value) { *reinterpret_cast<uint64_t*>(frame->page.data()) = value; };

uint64_t get_u64(BufferFrame* frame) { return *reinterpret_cast<uint64_t*>(frame->page.data()); };

void BaseTest::SetUp() {
  InitVars();

  // Clean up before test in case it crashed last time
  std::error_code ec;
  std::filesystem::remove_all(_base_dir_ssd, ec);
  std::filesystem::create_directories(_base_dir_ssd);

  _ssd_path = _base_dir_ssd / "buffer_manager_ssd.data";
}

void BaseTest::TearDown() {
  std::error_code ec;
  // std::filesystem::remove_all(base_dir_ssd, ec);
}

std::unique_ptr<BufferManager> BaseTest::create_default_bm() {
  return std::make_unique<BufferManager>(std::make_unique<VolatileRegion>(_frame_count),
                                         std::make_unique<SSDRegion>(_ssd_path, _page_count));
}
