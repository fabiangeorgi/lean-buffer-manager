#include "buffer_manager.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <random>
#include <iostream>

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
    _create_cooling_state_share();
    return bf;
}

void BufferManager::free_page(BufferFrame *frame) {
    // TODO(student) implement
    // use this order because otherwise the page_id is INVALID
    // in the volatile region we directly overwrite at the frame memory addresss
    // thus when reading from it again we get not the correct page id back
    _ssd_region->free_page_id(frame->page_id);
    _volatile_region->free_frame(frame);
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
        _remove_eviction_candidate(swip.buffer_frame());
        // TODO maybe check that it does not directly get back to cooling
        _create_cooling_state_share();
        return swip.buffer_frame();
    }
        // Resolve evicted Swip
    else {
        auto pageId = swip.page_id();
        if (_volatile_region->free_frame_count() == 0) {
            _evict_page();
        }

        auto *bf = _volatile_region->allocate_frame();
        bf->page_id = pageId;
        _ssd_region->read_page(bf->page.data(), pageId);
        swip.swizzle(bf);

        _create_cooling_state_share();
        return bf;
    }
    // Ensure the number of cooling frames if a frame was allocated in this function since this allocation might have
    // triggered an eviction.
}

void BufferManager::register_callbacks(Callbacks &&callbacks) { _callbacks = std::move(callbacks); }

void BufferManager::register_data_structure(ManagedDataStructure *data_structure) {
    _managed_data_structure = data_structure;
}

void BufferManager::_flush(BufferFrame *frame) {
    // TODO(student)
    _ssd_region->write_page(frame->page.data(), frame->page_id);
    frame->mark_written_back();
}

void BufferManager::_evict_page() {
    // Flush the page if dirty. Set the page id for the swip pointing to the page. Free the frame.
    // TODO(student)
    auto *bf = _pop_eviction_candidate();
    if (bf->is_dirty()) {
        _flush(bf);
    }

    auto swip = _callbacks.get_parent(bf, _managed_data_structure);
    swip.evict(bf->page_id);
    _volatile_region->free_frame(bf);
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
    // only add if not eviction candidate
    // TODO: check later: can we remove this here and move this condition somewhere else?
    if (!_has_eviction_candidate(frame)) {
        std::cout << "Push to eviciton page: " << frame->page_id << std::endl;
        eviction_candidates.push_back(frame);
    }
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

void BufferManager::_create_cooling_state_share() {
    // TODO somewhere here is a endless loop
    auto const frameCount = _volatile_region->frame_count();
    auto const framesNeededInCoolingStage = static_cast<uint64_t>(frameCount * SHARE_COOLING_PAGES);
    auto const fiftyPercentOfFrames =  static_cast<uint64_t>(frameCount * SHARE_USED_PAGES_BEFORE_COOLING);
    // TODO we can later change this logic to just use free frame count, but for now makes logic easier to implement
    auto const currentlyUsedFrames = frameCount - _volatile_region->free_frame_count();

    if (currentlyUsedFrames < fiftyPercentOfFrames) {
        // we don't have the needed amount of frames for things to be cooled
        return;
    }

    // if we do -> add as much to cooling state as we need to reach quota
    while (_eviction_candidate_count() < framesNeededInCoolingStage) {
        auto eviction_candidate = _random_frame();
        auto swip = _callbacks.get_parent(eviction_candidate, _managed_data_structure);
        // if swip is not hot -> already evicted, cooling or free -> get new random frame
        while (!swip.is_swizzled() && !_has_eviction_candidate(swip.buffer_frame())) {
            eviction_candidate = _random_frame();
            swip = _callbacks.get_parent(eviction_candidate, _managed_data_structure);
        }

        // we found a hot one -> check if all its children are not hot -> then we can use it
        // otherwise use the children -> children might need to propagate down again
        Swip &iterator = swip;
        // when deleting: children could already be not evicted
        auto childrenIsSwizzledIteratorFunction = [&iterator, this](Swip &swip) {
            if (swip.is_swizzled() && !this->_has_eviction_candidate(swip.buffer_frame())) {
                std::cout << "Swip is swizzled: " << swip.buffer_frame()->page_id << std::endl;
                iterator = swip;
                return true;
            }
            return false;
        };

        while (true) {
            bool atleastOneChildrenIsSwizzled = _callbacks.iterate_children(swip.buffer_frame(),
                                                                            childrenIsSwizzledIteratorFunction);
            // check that at least one child is swizzled
            if (!atleastOneChildrenIsSwizzled) {
                // we found one candidate -> thus we can add it to the eviction candidates and unswizzle its pointer
                _add_eviction_candidate(iterator.buffer_frame());
                iterator.unswizzle();
                break;
            }
        }
    }
}
