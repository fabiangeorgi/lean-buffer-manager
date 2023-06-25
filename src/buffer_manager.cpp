#include "buffer_manager.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <random>

#include "buffer_frame.hpp"
#include "swip.hpp"

BufferManager::BufferManager(std::unique_ptr<VolatileRegion> volatile_region, std::unique_ptr<SSDRegion> ssd_region)
    : _volatile_region(std::move(volatile_region)), _ssd_region(std::move(ssd_region)) {
  // Do not modify the lines below.
  _random_generator.seed(42);
  _distribution = std::uniform_int_distribution<uint64_t>(0, _volatile_region->frame_count() - 1);

  // -- TODO(student) implement
  // ...
}

BufferFrame* BufferManager::allocate_page() {
  // TODO(student)
  return nullptr;
}

void BufferManager::free_page(BufferFrame* frame) {
  // TODO(student) implement
}

BufferFrame* BufferManager::get_frame(Swip& swip) {
  // -- TODO(student) implement
  // Resolve swizzled Swip

  // Resolve cooling Swip

  // Resolve evicted Swip

  // Ensure the number of cooling frames if a frame was allocated in this function since this allocation might have
  // triggered an eviction.
  return nullptr;
}

void BufferManager::register_callbacks(Callbacks&& callbacks) { _callbacks = std::move(callbacks); }

void BufferManager::register_data_structure(ManagedDataStructure* data_structure) {
  _managed_data_structure = data_structure;
}

void BufferManager::_flush(BufferFrame* frame) {
  // TODO(student)
}

void BufferManager::_evict_page() {
  // Flush the page if dirty. Set the page id for the swip pointing to the page. Free the frame.
  // TODO(student)
}

bool BufferManager::_has_eviction_candidate(const BufferFrame* frame) {
  // TODO(student) implement
  return false;
}

BufferFrame* BufferManager::_pop_eviction_candidate() {
  // TODO(student) implement
  return nullptr;
}

void BufferManager::_add_eviction_candidate(BufferFrame* frame) {
  // TODO(student) implement
}

void BufferManager::_remove_eviction_candidate(const BufferFrame* frame) {
  // TODO(student) implement
}

uint32_t BufferManager::_eviction_candidate_count() {
  // TODO(student) implement
  return 0;
}

BufferFrame* BufferManager::_random_frame() {
  // Do not modify.
  const uint64_t random_frame_offset = _distribution(_random_generator);
  return _volatile_region->frames() + random_frame_offset;
}
