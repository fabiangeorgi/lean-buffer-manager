#include "data_regions.hpp"

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

///////////////////////////////////////////////////////////
//// Volatile Region
///////////////////////////////////////////////////////////

VolatileRegion::VolatileRegion(uint64_t frame_count) : _frame_count{frame_count} {
  // -- TODO(student) get a memory region for volatile buffer frames.

  // TODO(student) uncomment `init_free_frames` call in line below once data is allocated.
  // _init_free_frames();
}

VolatileRegion::~VolatileRegion() {
  // TODO(student) free memory
}

BufferFrame* VolatileRegion::allocate_frame() {
  // Enough memory frames need to be allocated. Thus, you need to ensure that at least one frame is available before
  // calling this function. The buffer manager ensures this with the eviction policy.

  // -- TODO(student) implement
  return nullptr;
}

void VolatileRegion::free_frame(BufferFrame* frame) {
  // -- TODO(student) manage the region's state so that this frame is indicated as free now. Thus, it can be used and
  // returned in `allocate_frame`.
}

BufferFrame* VolatileRegion::frames() {
  // -- TODO(student) return a pointer pointing the start address of the volatile region as BufferFrame*.
  return nullptr;
}

uint64_t VolatileRegion::frame_count() const {
  // -- TODO(student) return the total number of frames.
  return 0;
}

uint64_t VolatileRegion::free_frame_count() const {
  // -- TODO(student) return the number of free frames.
  return 0;
}

void VolatileRegion::_init_free_frames() {
  auto frames_begin = frames();

  // Initialize all frames. You can use `new (frames_begin + frame_offset) BufferFrame()` to place the buffer frames
  // directly into the pre-allocated storage at memory address `frames_begin + frame_offset`. For more details, search
  // for `Placement new` in cppreference.

  // -- TODO(student)
}

///////////////////////////////////////////////////////////
//// SSD Region
///////////////////////////////////////////////////////////

// TODO(student) implement
SSDRegion::SSDRegion(const std::filesystem::path& file_path, uint64_t page_count) {
  // open the file at `file_path`. Note, we want to read and write data. If the file already exists, overwrite it. Note,
  // that O_DIRECT is required.
  _init_free_pages();
}

uint64_t SSDRegion::page_count() const {
  // TODO(student) implement
  return 0;
}

uint64_t SSDRegion::free_page_count() const {
  // TODO(student) implement
  return 0;
}

PageID SSDRegion::allocate_page_id() {
  // TODO(student) implement
  return 0;
}

void SSDRegion::free_page_id(PageID page_id) {
  // TODO(student) implement
}

SSDRegion::~SSDRegion() {
  // TODO(student) free resources
}

void SSDRegion::read_page(std::byte* destination, PageID page_id) {
  // TODO(student) read data from page at `page_id` into `destination`.
}

void SSDRegion::write_page(const std::byte* source, PageID page_id) {
  // TODO(student) write data from page at `source` into the page with `page_id` on disk. You need to explicitly flush
  // data when writing to disk, as this is otherwise not guaranteed to be persistent in case of power loss.
}

void SSDRegion::_init_free_pages() {
  // -- TODO(student) setup required variables that you might use to fulfill the requirement of `allocate_page_id`,
  // i.e., page IDs are ascending, starting with 0, but a most recently freed page ID is allocated first.
}
