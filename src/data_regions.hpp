#pragma once

#include <cstdint>
#include <filesystem>
#include <stack>
#include <vector>

#include "buffer_frame.hpp"

class BufferManager;

class VolatileRegion {
public:
    // Allocates memory for volatile BufferFrames, i.e., this memory range holds all data that you will read and modify
    // while in memory. This is the in-memory part of the buffer pool. Note: The memory allocated here must be aligned
    // for BufferFrames, i.e., we must be able to create a BufferFrame at the 0th byte of the data. With the current
    // implementation of the utility functions data_begin() and data_end() (see below), the member variables _data needs
    // to store the start address of the allocated memory region. If you do not store the start address pointer in _data,
    // you need to modify data_begin() and data_end() accordingly.
    explicit VolatileRegion(uint64_t frame_count);

    // Free all acquired resources.
    ~VolatileRegion();

    // Create a new frame within the volatile region to use. The function assumes that at least one free frame is
    // available. Thus, the caller has to make sure that at least one frame is free.
    BufferFrame *allocate_frame();

    // Frees the memory of the volatile `frame`. Thus, the frame's memory region can be reused for other frame allocations
    // afterwards. Note that modified pages stored in a buffer frame should be flushed before calling this function.
    // Otherwise, the data changes are lost.
    void free_frame(BufferFrame *frame);

    // Returns the pointer to the volatile data/memory region as a BufferFrame*. This allows accessing all
    // of the volatile region's frames.
    BufferFrame *frames();

    // Returns the total number of volatile data region's frames.
    uint64_t frame_count() const;

    // Returns the number of free frames in the volatile data region.
    uint64_t free_frame_count() const;

    // Helper methods for tests. Do not modify!
    bool address_in_range(const void *addr) const { return data_begin() <= addr && addr < data_end(); }

    std::byte *data_begin() const { return _data; }

    std::byte *data_end() const { return _data + _frame_count * sizeof(BufferFrame); }

    // Delete move and copy
    VolatileRegion(const VolatileRegion &) = delete;

    VolatileRegion(VolatileRegion &&) = delete;

    VolatileRegion &operator=(const VolatileRegion &) = delete;

    VolatileRegion &operator=(VolatileRegion &&) = delete;

private:
    void _init_free_frames();

    std::byte *_data = nullptr;
    const uint64_t _frame_count;
    uint64_t _free_frame_count;
    std::vector<BufferFrame *> _free_frames{};
};

// This class represents the data on disk. Pages are logically mapped from 0 to n via their page id. So page 0 starts
// at offset 0, page 1 starts at 1 * PAGE_SIZE, page 2 starts at 2 * PAGE_SIZE, and page k starts at k * PAGE_SIZE.
class SSDRegion {
public:
    // Opens the file at `file_path`. Overwrite an existing file and resize it to contain `page_count` pages.
    SSDRegion(const std::filesystem::path &file_path, uint64_t page_count);

    // Free all acquired resources.
    ~SSDRegion();

    // Returns the next available page ID. Page IDs are ascending starting with 0. However, page IDs can be freed, when
    // they are not required anymore. If page IDs get freed, this function returns the most recently freed page id.
    PageID allocate_page_id();

    // Frees the given page_id so that it is available for allocation (see `allocate_page_id`).
    void free_page_id(PageID page_id);

    // Reads an entire page (= PAGE_SIZE) with `page_id` from the backing file into `destination`.
    void read_page(std::byte *destination, PageID page_id);

    // Writes an entire page (= PAGE_SIZE) with `page_id` from `source` to the backing file.
    void write_page(const std::byte *source, PageID page_id);

    // Returns the total number of the SSD data region's pages (including unwritten ones).
    uint64_t page_count() const;

    // Returns the number of available, i.e., currently not allocated, pages in the SSD data region.
    uint64_t free_page_count() const;

    // Delete move and copy
    SSDRegion(const SSDRegion &) = delete;

    SSDRegion(SSDRegion &&) = delete;

    SSDRegion &operator=(const SSDRegion &) = delete;

    SSDRegion &operator=(SSDRegion &&) = delete;

private:
    void _init_free_pages();

    // TODO(student) Feel free to add further variables or functions if needed.
    // ...
    int32_t _file;
    uint64_t _page_count;
    std::deque<PageID> _free_pages{};
};
