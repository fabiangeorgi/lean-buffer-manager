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
    // TODO check flags
    _data = reinterpret_cast<std::byte *>(mmap(nullptr, frame_count * sizeof(BufferFrame), PROT_READ | PROT_WRITE,
                                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    madvise(_data, frame_count * sizeof(BufferFrame), MADV_HUGEPAGE);
    // TODO(student) uncomment `init_free_frames` call in line below once data is allocated.
    _init_free_frames();
}

VolatileRegion::~VolatileRegion() {
    // TODO(student) free memory
    munmap(frames(), frame_count() * sizeof(BufferFrame));
}

BufferFrame *VolatileRegion::allocate_frame() {
    // Enough memory frames need to be allocated. Thus, you need to ensure that at least one frame is available before
    // calling this function. The buffer manager ensures this with the eviction policy.

    // -- TODO(student) implement
    auto value = _free_frames[0];
    _free_frames.erase(_free_frames.begin());
    _free_frame_count--;
    return value;
}

void VolatileRegion::free_frame(BufferFrame *frame) {
    // -- TODO(student) manage the region's state so that this frame is indicated as free now. Thus, it can be used and
    // returned in `allocate_frame`.
    _free_frames.insert(_free_frames.begin(), new(frame) BufferFrame());
    _free_frame_count++;
}

BufferFrame *VolatileRegion::frames() {
    // -- TODO(student) return a pointer pointing the start address of the volatile region as BufferFrame*.
    return reinterpret_cast<BufferFrame *>(_data);
}

uint64_t VolatileRegion::frame_count() const {
    // -- TODO(student) return the total number of frames.
    return _frame_count;
}

uint64_t VolatileRegion::free_frame_count() const {
    return _free_frame_count;
}

void VolatileRegion::_init_free_frames() {
    auto frames_begin = frames();

    // Initialize all frames. You can use `new (frames_begin + frame_offset) BufferFrame()` to place the buffer frames
    // directly into the pre-allocated storage at memory address `frames_begin + frame_offset`. For more details, search
    // for `Placement new` in cppreference.
    _free_frames.reserve(frame_count());

    for (auto i = 0; i < frame_count(); i++) {
        auto *bf = new(frames_begin + i) BufferFrame();
        _free_frames.push_back(bf);
    }

    _free_frame_count = _frame_count;
}

///////////////////////////////////////////////////////////
//// SSD Region
///////////////////////////////////////////////////////////

// TODO(student) implement
SSDRegion::SSDRegion(const std::filesystem::path &file_path, uint64_t page_count) : _page_count(page_count) {
    // open the file at `file_path`. Note, we want to read and write data. If the file already exists, overwrite it. Note,
    // that O_DIRECT is required.
    // open file
    // O_CREAT -> create if not exist
    // O_TRUNC -> TRUNCATE to 0 if exists
    // O_RDWR -> READ and WRITE
    // O_DIRECT -> direct disk access -> kernel does not get involved
    _file = open(file_path.c_str(), O_CREAT | O_RDWR | O_DIRECT | O_TRUNC, 0600);
    // should contain page_count pages
    auto dummy_data = (uint8_t *) aligned_alloc(sizeof(Page), page_count * sizeof(Page));
    pwrite(_file, dummy_data, page_count * sizeof(Page), 0);

    free(dummy_data);
    _init_free_pages();
}

uint64_t SSDRegion::page_count() const {
    // TODO(student) implement
    return _page_count;
}

uint64_t SSDRegion::free_page_count() const {
    // TODO(student) implement
    return _free_pages.size();
}

PageID SSDRegion::allocate_page_id() {
    PageID freePageId = _free_pages.front();
    _free_pages.pop_front();
    return freePageId;
}

void SSDRegion::free_page_id(PageID page_id) {
    // TODO(student) implement
    _free_pages.push_front(page_id);
}

SSDRegion::~SSDRegion() {
    // TODO(student) free resources
    close(_file);
}

void SSDRegion::read_page(std::byte *destination, PageID page_id) {
    // TODO(student) read data from page at `page_id` into `destination`.
    pread(_file, destination, sizeof(Page), page_id * sizeof(Page));
}

void SSDRegion::write_page(const std::byte *source, PageID page_id) {
    // TODO(student) write data from page at `source` into the page with `page_id` on disk. You need to explicitly flush
    // data when writing to disk, as this is otherwise not guaranteed to be persistent in case of power loss.
    pwrite(_file, source, sizeof(Page), page_id * sizeof(Page));
    fsync(_file);
}

void SSDRegion::_init_free_pages() {
    // -- TODO(student) setup required variables that you might use to fulfill the requirement of `allocate_page_id`,
    // i.e., page IDs are ascending, starting with 0, but a most recently freed page ID is allocated first.
    for (PageID i = 0; i < _page_count; i++) {
        _free_pages.push_back(i);
    }
}
