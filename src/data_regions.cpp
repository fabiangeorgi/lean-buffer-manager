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
    _data = reinterpret_cast<std::byte *>(mmap(nullptr, frame_count * sizeof(BufferFrame), PROT_READ | PROT_WRITE,
                                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    madvise(_data, frame_count * sizeof(BufferFrame), MADV_HUGEPAGE);
    _init_free_frames();
}

VolatileRegion::~VolatileRegion() {
    munmap(frames(), frame_count() * sizeof(BufferFrame));
}

BufferFrame *VolatileRegion::allocate_frame() {
    // Enough memory frames need to be allocated. Thus, you need to ensure that at least one frame is available before
    // calling this function. The buffer manager ensures this with the eviction policy.
    auto value = _free_frames.back();
    _free_frames.pop_back();
    return value;
}

void VolatileRegion::free_frame(BufferFrame *frame) {
    _free_frames.push_back(new (frame) BufferFrame());
}

BufferFrame *VolatileRegion::frames() {
    return reinterpret_cast<BufferFrame *>(_data);
}

uint64_t VolatileRegion::frame_count() const {
    return _frame_count;
}

uint64_t VolatileRegion::free_frame_count() const {
    return _free_frames.size();
}

void VolatileRegion::_init_free_frames() {
    auto frames_begin = frames();

    // Initialize all frames. You can use `new (frames_begin + frame_offset) BufferFrame()` to place the buffer frames
    // directly into the pre-allocated storage at memory address `frames_begin + frame_offset`. For more details, search
    // for `Placement new` in cppreference.
    _free_frames.reserve(frame_count());

    for (auto i = frame_count() - 1; i > 0; i--) {
        auto *bf = new(frames_begin + i) BufferFrame();
        _free_frames.push_back(bf);
    }
    _free_frames.push_back(new(frames_begin) BufferFrame());
}

///////////////////////////////////////////////////////////
//// SSD Region
///////////////////////////////////////////////////////////

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
    ftruncate(_file, page_count * sizeof(Page));

    _init_free_pages();
}

uint64_t SSDRegion::page_count() const {
    return _page_count;
}

uint64_t SSDRegion::free_page_count() const {
    return _free_pages.size();
}

PageID SSDRegion::allocate_page_id() {
    PageID freePageId = _free_pages.back();
    _free_pages.pop_back();
    return freePageId;
}

void SSDRegion::free_page_id(PageID page_id) {
    _free_pages.push_back(page_id);
}

SSDRegion::~SSDRegion() {
    close(_file);
}

void SSDRegion::read_page(std::byte *destination, PageID page_id) {
    pread(_file, destination, sizeof(Page), page_id * sizeof(Page));
}

void SSDRegion::write_page(const std::byte *source, PageID page_id) {
    pwrite(_file, source, sizeof(Page), page_id * sizeof(Page));
    fsync(_file);
}

void SSDRegion::_init_free_pages() {
    for (PageID i = _page_count - 1; i > 0; i--) {
        _free_pages.push_back(i);
    }
    _free_pages.push_back(0);
}
