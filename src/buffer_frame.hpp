#pragma once

#include <array>
#include <cstdint>
#include <limits>

using PageID = uint64_t;
static constexpr uint64_t KiB = 1024ul;
static constexpr uint64_t MiB = 1024 * KiB;
static constexpr uint64_t GiB = 1024 * MiB;
static constexpr uint64_t PAGE_SIZE = 4 * KiB;
// Data region to store the page's payload in. Note that other potential members of the page are also stored in a
// page, which is the data unit that gets persisted. Thus, if you add further member variables, the payload is
// smaller than PAGE_SIZE. In other words, the payload size is equal to: PAGE_SIZE - SIZE_OF_PAGE_MEMBERS.
static constexpr uint64_t SIZE_OF_PAGE_MEMBERS = 0;

// Note that we use the two least significant bits for tagging,
static constexpr PageID MAX_PAGE_ID = (std::numeric_limits<PageID>::max() >> 2) - 1;
static constexpr PageID INVALID_PAGE_ID = MAX_PAGE_ID + 1;

// Need 512 Byte alignment for O_DIRECT.
struct alignas(512) Page {
  std::array<std::byte, PAGE_SIZE - SIZE_OF_PAGE_MEMBERS> payload{};

  // Utility function that returns the start address of the stored page's data.
  std::byte* data();

  // Utility conversion function. If the page needs to be converted to a std::byte pointer (including all of its
  // member variables, not only the effective payload), this function does the job.
  operator std::byte*();
};

static constexpr uint64_t EFFECTIVE_PAGE_SIZE = sizeof(Page::payload);

// Frames are physically interleaved with the page content. This should improve locality by reducing the number of cache
// misses.
struct BufferFrame {
  BufferFrame() = default;
  // Sets a marker indicating that the corresponding page is dirty / modified. This is relevant for evicting a page:
  // only dirty pages need to be flushed to disk.
  void mark_dirty();

  // Sets a marker indicating that the corresponding page is written back / not dirty.
  void mark_written_back();

  // Checks whether the corresponding page is dirty / modified.
  bool is_dirty();

  // Utility function to cast the stored page's data to a T pointer. If you store an object of type T in a page, you can
  // use `frame_pointer->as<T>()` to get the data stored in the page's payload as a T pointer (T*).
  template <typename T>
  T* as() {
    return reinterpret_cast<T*>(page.data());
  }

  // Parent pointer is relevant if data needs to be unswizzled. This is managed by a buffer managed data structure.
  // You do not need to modify it.
  BufferFrame* parent_frame = nullptr;

  // Page ID of the corresponding page.
  PageID page_id = INVALID_PAGE_ID;

  // Actual page data.
  Page page{};

  // -- TODO(student) Add further functions and members if required.
  // ...
};
