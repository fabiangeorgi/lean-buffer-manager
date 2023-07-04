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
    // for performance -> resize and reserve vector
}

BufferFrame *BufferManager::allocate_page() {
    // TODO(student)
    if (_volatile_region->free_frame_count() == 0) {
        _evict_page();
    }

    auto pageId = _ssd_region->allocate_page_id();

    auto *bf = _volatile_region->allocate_frame();
    bf->page_id = pageId;

    return bf;
}

void BufferManager::free_page(BufferFrame *frame) {
    // TODO(student) implement
    _volatile_region->free_frame(frame);
    _ssd_region->free_page_id(frame->page_id);
}

BufferFrame *BufferManager::get_frame(Swip &swip) {
    // -- TODO(student) implement
    // Resolve swizzled Swip
    if (swip.is_swizzled()) {
        return swip.buffer_frame();
    }

    // Resolve cooling Swip
    else if (swip.is_cooling()) {
        swip.swizzle();
        // TODO 50% rule einhalten
        return swip.buffer_frame();
    }
    // Resolve evicted Swip
    else {
        auto pageId = swip.page_id();
        // TODO check before if enough frames allocated
        if (_volatile_region->free_frame_count() == 0) {
            _evict_page();
        }

        auto *bf = _volatile_region->allocate_frame();
        bf->page_id = pageId;
        _ssd_region->read_page(bf->page.data(), pageId);
        swip.swizzle(bf);
        return bf;
    }
    // Ensure the number of cooling frames if a frame was allocated in this function since this allocation might have
    // triggered an eviction.
    // TODO
}

void BufferManager::register_callbacks(Callbacks &&callbacks) { _callbacks = std::move(callbacks); }

void BufferManager::register_data_structure(ManagedDataStructure *data_structure) {
    _managed_data_structure = data_structure;
}

void BufferManager::_flush(BufferFrame *frame) {
    // TODO(student)
    _ssd_region->write_page(frame->page.data(), frame->page_id);
}

void BufferManager::_evict_page() {
    // Flush the page if dirty. Set the page id for the swip pointing to the page. Free the frame.
    // TODO(student)
    // TODO eviction logic
    auto *bf = _pop_eviction_candidate();
    if (bf->is_dirty()) {
        _flush(bf);
    }

    _volatile_region->free_frame(bf);
    auto swip = _callbacks.get_parent(bf, _managed_data_structure);
    swip.evict(bf->page_id);
}

bool BufferManager::_has_eviction_candidate(const BufferFrame *frame) {
    // TODO(student) implement
    return std::find(eviction_candidates.begin(), eviction_candidates.end(), frame) != eviction_candidates.end();
}

BufferFrame *BufferManager::_pop_eviction_candidate() {
    // TODO(student) implement
    auto *evictionCandidate = eviction_candidates.front();
    eviction_candidates.erase(eviction_candidates.begin());
    return evictionCandidate;
}

void BufferManager::_add_eviction_candidate(BufferFrame *frame) {
    // TODO(student) implement
    eviction_candidates.push_back(frame);
}

void BufferManager::_remove_eviction_candidate(const BufferFrame *frame) {
    // TODO(student) implement
    eviction_candidates.erase(std::remove(eviction_candidates.begin(), eviction_candidates.end(), frame),
                              eviction_candidates.end());
}

uint32_t BufferManager::_eviction_candidate_count() {
    // TODO(student) implement
    return eviction_candidates.size();
}

BufferFrame *BufferManager::_random_frame() {
    // Do not modify.
    const uint64_t random_frame_offset = _distribution(_random_generator);
    return _volatile_region->frames() + random_frame_offset;
}
