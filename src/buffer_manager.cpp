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
  // TODO don't know if volatile or ssd regions needs to be checked
  if (_volatile_region->free_frame_count() == 0) {
      // we manually need to evict a page from the eviction candidates
  }

  // here we can just allocate a new Bufferframe
  auto newPageId = _ssd_region->allocate_page_id();
  auto *newBf = _volatile_region->allocate_frame();
  newBf->page_id = newPageId;

  return newBf;
}

void BufferManager::free_page(BufferFrame* frame) {
  // TODO(student) implement
}

BufferFrame* BufferManager::get_frame(Swip& swip) {
  // -- TODO(student) implement
  // Resolve swizzled Swip
  if (swip.is_swizzled()) {
      // TODO oder auch mit den tags?
      return swip.buffer_frame();
  }

  // Resolve cooling Swip
  else if (swip.is_cooling()) {
      // TODO hier aus eviction candidate raus nehmen
      // cooling bits rausnehmen
      swip.swizzle();
      auto const *bf = swip.buffer_frame();

      // aus eviction candidates rausnehmen
      _remove_eviction_candidate(bf);
      // einen neuen hinzufügen TODO

      return swip.buffer_frame();
  }

  // Resolve evicted Swip
  else {
      PageID pageId = swip.page_id();
      auto* newBufferFrame = _volatile_region->allocate_frame();
      newBufferFrame->page_id = _ssd_region->allocate_page_id();
      _ssd_region->read_page(newBufferFrame->page.data(), pageId);
      swip.swizzle(newBufferFrame);
      return newBufferFrame;
  }

  // Ensure the number of cooling frames if a frame was allocated in this function since this allocation might have
  // triggered an eviction.
  // TODO
}

void BufferManager::register_callbacks(Callbacks&& callbacks) { _callbacks = std::move(callbacks); }

void BufferManager::register_data_structure(ManagedDataStructure* data_structure) {
  _managed_data_structure = data_structure;
}

void BufferManager::_flush(BufferFrame* frame) {
  // TODO(student)
  _ssd_region->write_page(frame->page.data(), frame->page_id);
  frame->mark_written_back();
}

void BufferManager::_evict_page() {
  // Flush the page if dirty. Set the page id for the swip pointing to the page. Free the frame.
  // TODO(student)
}

bool BufferManager::_has_eviction_candidate(const BufferFrame* frame) {
  // TODO(student) implement
  return eviction_candidates.contains(frame->page_id);
}

BufferFrame* BufferManager::_pop_eviction_candidate() {
  // TODO(student) implement
  // remove the first of the eviction candidates
  auto * bf = eviction_candidates.begin()->second;
  _remove_eviction_candidate(bf);
  return bf;
}

void BufferManager::_add_eviction_candidate(BufferFrame* frame) {
  // TODO(student) implement
  eviction_candidates.insert({frame->page_id, frame});
}

void BufferManager::_remove_eviction_candidate(const BufferFrame* frame) {
  // TODO(student) implement
  eviction_candidates.erase(frame->page_id);
}

uint32_t BufferManager::_eviction_candidate_count() {
  // TODO(student) implement
  return eviction_candidates.size();
}

BufferFrame* BufferManager::_random_frame() {
  // Do not modify.
  const uint64_t random_frame_offset = _distribution(_random_generator);
  return _volatile_region->frames() + random_frame_offset;
}
