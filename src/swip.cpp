#include "swip.hpp"

#include "buffer_frame.hpp"

Swip::Swip() : pageId(INVALID_PAGE_ID << NUMBER_OF_BITS_FOR_TAGGING | evictedBits) {}

Swip::Swip(PageID page_id) : pageId(page_id << NUMBER_OF_BITS_FOR_TAGGING | evictedBits) {}

Swip::Swip(BufferFrame *buffer_frame) : pBufferFrame(reinterpret_cast<BufferFrame *>(reinterpret_cast<uint64_t>(buffer_frame) << NUMBER_OF_BITS_FOR_TAGGING)) {}

bool Swip::is_swizzled() {
    return (pageId & comparisonMask) == hotBits;
};

bool Swip::is_cooling() {
    return (pageId & comparisonMask) == coolingBits;
}

bool Swip::is_evicted() {
    return (pageId & comparisonMask) == evictedBits;
}

void Swip::swizzle() {
    this->pageId = this->pageId & ~coolingBits;
}

void Swip::swizzle(BufferFrame *buffer_frame) {
    this->pBufferFrame = reinterpret_cast<BufferFrame *>(reinterpret_cast<uint64_t>(buffer_frame)
            << NUMBER_OF_BITS_FOR_TAGGING);
}

void Swip::unswizzle() {
    this->pageId = this->pageId | coolingBits;
}

void Swip::evict(PageID page_id) {
    this->pageId = ((page_id << NUMBER_OF_BITS_FOR_TAGGING) | evictedBits);
}

PageID Swip::page_id() {
    return (this->pageId >> NUMBER_OF_BITS_FOR_TAGGING);
}

BufferFrame *Swip::buffer_frame() {
    // we use the pageId here because otherwise we cant shift it
    // TODO this does not make sense -> this is totally stupid from the test setup perspective -> because I would like to just return the shifted version
    if (this->is_swizzled()) {
        return reinterpret_cast<BufferFrame *>(this->pageId >> NUMBER_OF_BITS_FOR_TAGGING);
    } else {
        return this->pBufferFrame;
    }
}

BufferFrame *Swip::buffer_frame_ignore_tags() {
    return reinterpret_cast<BufferFrame *>(this->pageId >> NUMBER_OF_BITS_FOR_TAGGING);
}
