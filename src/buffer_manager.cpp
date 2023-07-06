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
        : _volatile_region(std::move(volatile_region)), _ssd_region(std::move(ssd_region)),
        FRAME_COUNT_MAX( _volatile_region->frame_count()),
        FRAMES_NEEDED_IN_COOLING_STAGE(static_cast<uint64_t>(FRAME_COUNT_MAX * SHARE_COOLING_PAGES)),
        FIFTY_PERCENT_FRAMES(static_cast<uint64_t>(FRAME_COUNT_MAX * SHARE_USED_PAGES_BEFORE_COOLING)) {
    // Do not modify the lines below.
    _random_generator.seed(42);
    _distribution = std::uniform_int_distribution<uint64_t>(0, _volatile_region->frame_count() - 1);

    // ...
}

BufferFrame *BufferManager::allocate_page() {
    if (_volatile_region->free_frame_count() == 0) {
        _evict_page();
    }

    auto *bf = _volatile_region->allocate_frame();
    auto pageId = _ssd_region->allocate_page_id();
    bf->page_id = pageId;
    _create_cooling_state_share(bf);
    return bf;
}

void BufferManager::free_page(BufferFrame *frame) {
    // use this order because otherwise the page_id is INVALID
    // in the volatile region we directly overwrite at the frame memory addresss
    // thus when reading from it again we get not the correct page id back
    _ssd_region->free_page_id(frame->page_id);
    _volatile_region->free_frame(frame);
}

BufferFrame *BufferManager::get_frame(Swip &swip) {
    // Resolve swizzled Swip
    if (swip.is_swizzled()) {
        return swip.buffer_frame();
    }

        // Resolve cooling Swip
    else if (swip.is_cooling()) {
        swip.swizzle();
        _remove_eviction_candidate(swip.buffer_frame());
        _create_cooling_state_share(swip.buffer_frame());
        return swip.buffer_frame();
    }
        // Resolve evicted Swip
    else {
        if (_volatile_region->free_frame_count() == 0) {
            _evict_page();
        }

        auto *bf = _volatile_region->allocate_frame();
        _create_cooling_state_share(bf);

        auto pageId = swip.page_id();
        bf->page_id = pageId;
        swip.swizzle(bf);
        _ssd_region->read_page(bf->page.data(), pageId);

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
    _ssd_region->write_page(frame->page.data(), frame->page_id);
    frame->mark_written_back();
}

void BufferManager::_evict_page() {
    // Flush the page if dirty. Set the page id for the swip pointing to the page. Free the frame.
    auto *bf = _pop_eviction_candidate();
    if (bf->is_dirty()) {
        _flush(bf);
    }

    if (_callbacks.get_parent) {
        auto& swip = _callbacks.get_parent(bf, _managed_data_structure);
        swip.evict(bf->page_id);
    }

    _volatile_region->free_frame(bf);
}

bool BufferManager::_has_eviction_candidate(BufferFrame *frame) {
    return fast_access.contains(frame);
}

BufferFrame *BufferManager::_pop_eviction_candidate() {
    auto begin = eviction_list.begin();
    fast_access.erase(*begin);
    eviction_list.erase(begin);
    return *begin;
}

void BufferManager::_add_eviction_candidate(BufferFrame *frame) {
    if (!_has_eviction_candidate(frame)) {

        fast_access[frame] = eviction_list.insert(eviction_list.end(), frame);

        if (_callbacks.get_parent) {
            Swip& swip = _callbacks.get_parent(frame, _managed_data_structure);
            swip.unswizzle();
        }
    }
}

void BufferManager::_remove_eviction_candidate(BufferFrame *frame) {
    if (auto element = fast_access.find(frame); element != fast_access.end()) {
        eviction_list.erase(element->second);
        fast_access.erase(element->first);
    }
}

uint32_t BufferManager::_eviction_candidate_count() {
    return eviction_list.size();
}

BufferFrame *BufferManager::_random_frame() {
    // Do not modify.
    const uint64_t random_frame_offset = _distribution(_random_generator);
    return _volatile_region->frames() + random_frame_offset;
}

void BufferManager::_create_cooling_state_share(const BufferFrame* bf) {
    // check if currently used frames = FRAME_COUNT_MAX - _volatile_region->free_frame_count() smaller than we need
    if (FRAME_COUNT_MAX - _volatile_region->free_frame_count() < FIFTY_PERCENT_FRAMES) {
        // we don't have the needed amount of frames for things to be cooled
        return;
    }

    // if we do -> add as much to cooling state as we need to reach quota
    while (_eviction_candidate_count() < FRAMES_NEEDED_IN_COOLING_STAGE) {
        auto eviction_candidate = _random_frame();
        // if swip is not hot -> already evicted, cooling or free -> get new random frame

        if (eviction_candidate == bf || eviction_candidate->page_id == INVALID_PAGE_ID) {
            continue;
        }

        if (!_callbacks.iterate_children) {
            _add_eviction_candidate(eviction_candidate);
            continue;
        }

        // we found a hot one -> check if all its children are not hot -> then we can use it
        // otherwise use the children -> children might need to propagate down again
        // when deleting: children could already be not evicted
        auto childrenIsSwizzledIteratorFunction = [&eviction_candidate](Swip &swip) {
            if (swip.is_swizzled()) {
                eviction_candidate = swip.buffer_frame();
                return true;
            }
            return false;
        };

        while (true) {
            bool atleastOneChildrenIsSwizzled = _callbacks.iterate_children(eviction_candidate,
                                                                            childrenIsSwizzledIteratorFunction);
            // check that at least one child is swizzled
            if (!atleastOneChildrenIsSwizzled) {
                // we found one candidate -> thus we can add it to the eviction candidates and unswizzle its pointer
                _add_eviction_candidate(eviction_candidate);
                break;
            }
        }
    }
}
