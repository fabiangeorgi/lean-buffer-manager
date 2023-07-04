#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <bitset>

#include "buffer_frame.hpp"
#include "buffer_manager.hpp"
#include "gtest/gtest.h"
#include "test_utils.hpp"

class BasicTest : public BaseTest {
protected:
    void InitVars() override {
        // 1 MiB memory pool
        _frame_count = 256;
        // 2 MiB on SSD
        _page_count = 512;

        _base_dir_ssd = "/tmp/hdp23";
    }
};

///////////////////////////////////////////////////////////
//// Buffer Frame
///////////////////////////////////////////////////////////

class BufferFrameTest : public BasicTest {
};

TEST_F(BufferFrameTest, IsDirty) {
    BufferFrame frame{};
    EXPECT_FALSE(frame.is_dirty());
    frame.mark_dirty();
    EXPECT_TRUE(frame.is_dirty());
    frame.mark_written_back();
    EXPECT_FALSE(frame.is_dirty());
}

TEST_F(BufferFrameTest, PageSize) {
    EXPECT_EQ(sizeof(Page), 4096);
    EXPECT_EQ(sizeof(Page), PAGE_SIZE);
    EXPECT_EQ(PAGE_SIZE, EFFECTIVE_PAGE_SIZE - SIZE_OF_PAGE_MEMBERS);
}

TEST_F(BufferFrameTest, AllocateFrame) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    BufferFrame *frame = buffer_manager->allocate_page();
    ASSERT_NE(frame, nullptr);
    EXPECT_EQ((frame->page_id), PageID{0});
    frame = buffer_manager->allocate_page();
    EXPECT_EQ(frame->page_id, PageID{1});
    frame = buffer_manager->allocate_page();
    EXPECT_EQ(frame->page_id, PageID{2});

    new(frame->page.data()) uint64_t{42};
    new(frame->page.data() + 8) uint32_t{128};
    new(frame->page.data() + 16) uint16_t{14};

    const uint64_t *val_a = reinterpret_cast<uint64_t *>(frame->page.data());
    const uint32_t *val_b = reinterpret_cast<uint32_t *>(frame->page.data() + 8);
    const uint16_t *val_c = reinterpret_cast<uint16_t *>(frame->page.data() + 16);
    EXPECT_EQ(*val_a, uint64_t{42});
    EXPECT_EQ(*val_b, uint64_t{128});
    EXPECT_EQ(*val_c, uint64_t{14});
}

TEST_F(BufferFrameTest, As) {
    struct TestStruct {
        float val;
    };

    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    BufferFrame *frame = buffer_manager->allocate_page();
    ASSERT_NE(frame, nullptr);
    auto expected_object = new(frame->page.data()) TestStruct();
    expected_object->val = 42.f;

    auto actual_object = frame->as<TestStruct>();
    EXPECT_EQ(typeid(expected_object), typeid(actual_object));
    EXPECT_EQ(expected_object, actual_object);
    EXPECT_EQ(expected_object->val, actual_object->val);
}

TEST_F(BufferFrameTest, Data) {
    using Key = uint64_t;
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    BufferFrame *frame = buffer_manager->allocate_page();
    EXPECT_EQ(frame->page.payload.data(), frame->page.data());
}

///////////////////////////////////////////////////////////
//// Swip
//////////////////////////////////////////////////////////

class SwipTest : public BasicTest {
};

TEST_F(SwipTest, HasCorrectSize) { EXPECT_EQ(sizeof(Swip), 8); }

TEST_F(SwipTest, ConstructorFrameSwip) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    BufferFrame *frame = buffer_manager->allocate_page();

    {
        auto swip = Swip(frame);
        EXPECT_TRUE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_FALSE(swip.is_evicted());
        EXPECT_EQ(swip.buffer_frame(), frame);
    }

    {
        auto swip = Swip(frame);
        swip.unswizzle();
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_TRUE(swip.is_cooling());
        EXPECT_FALSE(swip.is_evicted());
        EXPECT_NE(swip.buffer_frame(), frame);
        EXPECT_EQ(swip.buffer_frame_ignore_tags(), frame);
    }

    {
        auto page_id = PageID{1337};
        auto swip = Swip(page_id);
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_TRUE(swip.is_evicted());
        EXPECT_EQ(swip.page_id(), page_id);
    }

    {
        auto page_id = PageID{1336};
        auto swip = Swip(page_id);
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_TRUE(swip.is_evicted());
        EXPECT_EQ(swip.page_id(), page_id);
    }

    // complex test
    {
        auto page_id = PageID{1336};
        auto swip = Swip(page_id);
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_TRUE(swip.is_evicted());
        EXPECT_EQ(swip.page_id(), page_id);
        // swizzle it
        swip.swizzle(frame);
        EXPECT_TRUE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_FALSE(swip.is_evicted());
        EXPECT_EQ(swip.buffer_frame(), frame);

        swip.evict(7);
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_TRUE(swip.is_evicted());
        EXPECT_EQ(swip.page_id(), 7);
    }

    {
        auto swip = Swip();
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_TRUE(swip.is_evicted());
        EXPECT_EQ(swip.page_id(), INVALID_PAGE_ID);
    }

    {
        auto swip = Swip(frame);
        swip.unswizzle();
        EXPECT_FALSE(swip.is_swizzled());
        EXPECT_TRUE(swip.is_cooling());
        EXPECT_FALSE(swip.is_evicted());
        EXPECT_NE(swip.buffer_frame(), frame);
        EXPECT_EQ(swip.buffer_frame_ignore_tags(), frame);

        // now swizzle again
        swip.swizzle();
        EXPECT_TRUE(swip.is_swizzled());
        EXPECT_FALSE(swip.is_cooling());
        EXPECT_FALSE(swip.is_evicted());
        EXPECT_EQ(swip.buffer_frame(), frame);
        EXPECT_EQ(swip.buffer_frame_ignore_tags(), frame);
    }
}

TEST_F(SwipTest, PointerLayout) {
    // x00
    static const uint64_t hotBits = uint64_t(0);
    // x10
    static const uint64_t coolingBits = uint64_t(2);
    // x01
    static const uint64_t evictedBits = uint64_t(1);

    static const uint64_t coolingMask = ~coolingBits;
    static const uint64_t hotMask = ~hotBits;
    static const uint64_t evictedMask = ~evictedBits;
    std::cout << std::bitset<64>(hotBits) << std::endl;
    std::cout << std::bitset<64>(coolingBits) << std::endl;
    std::cout << std::bitset<64>(evictedBits) << std::endl;
    std::cout << std::bitset<64>(hotMask) << std::endl;
    std::cout << std::bitset<64>(coolingMask) << std::endl;
    std::cout << std::bitset<64>(evictedMask) << std::endl;

    std::cout << std::bitset<64>(MAX_PAGE_ID) << std::endl;
    std::cout << std::bitset<64>(MAX_PAGE_ID << 2) << std::endl;
}

///////////////////////////////////////////////////////////
//// Data Region
///////////////////////////////////////////////////////////

class VolatileDataRegionTest : public BasicTest {
};

class SSDDataRegionTest : public BasicTest {
};

TEST_F(VolatileDataRegionTest, AddressInRange) {
    VolatileRegion region{30};
    ASSERT_NE(region.data_begin(), nullptr);
    const std::byte *volatile_data = region.data_begin() + 10;
    EXPECT_TRUE(region.address_in_range(volatile_data));

    const std::byte *not_in_range = region.data_end() + 1;
    EXPECT_FALSE(region.address_in_range(not_in_range));
}

TEST_F(VolatileDataRegionTest, AllocateFrame) {
    VolatileRegion region{30};
    ASSERT_EQ(region.frame_count(), 30);
    ASSERT_EQ(region.free_frame_count(), 30);

    BufferFrame *frames = region.frames();
    for (uint32_t frame_idx = 0; frame_idx < region.frame_count(); ++frame_idx) {
        ASSERT_NE(frames + frame_idx, nullptr);
    }

    BufferFrame *frame_0 = region.allocate_frame();
    EXPECT_NE(frame_0, nullptr);
    EXPECT_EQ(frame_0, frames + 0);
    EXPECT_TRUE(region.address_in_range(frame_0));

    BufferFrame *frame_1 = region.allocate_frame();
    EXPECT_NE(frame_1, nullptr);
    EXPECT_EQ(frame_1, frames + 1);
    EXPECT_TRUE(region.address_in_range(frame_1));

    BufferFrame *frame_2 = region.allocate_frame();
    EXPECT_NE(frame_2, nullptr);
    EXPECT_EQ(frame_2, frames + 2);
    EXPECT_TRUE(region.address_in_range(frame_1));

    EXPECT_EQ(region.free_frame_count(), 27);
    frame_0->page_id = 15;
    frame_1->page_id = 20;
    frame_2->page_id = 10;
    region.free_frame(frame_0);
    EXPECT_EQ(frame_0->page_id, INVALID_PAGE_ID);
    EXPECT_EQ(region.free_frame_count(), 28);
    region.free_frame(frame_1);
    EXPECT_EQ(frame_1->page_id, INVALID_PAGE_ID);
    EXPECT_EQ(region.free_frame_count(), 29);
    region.free_frame(frame_2);
    EXPECT_EQ(frame_2->page_id, INVALID_PAGE_ID);
    EXPECT_EQ(region.free_frame_count(), 30);
}

// own test
TEST_F(VolatileDataRegionTest, AllocateAndFreeFrames) {
    VolatileRegion region{30};
    ASSERT_EQ(region.frame_count(), 30);
    ASSERT_EQ(region.free_frame_count(), 30);

    BufferFrame *frames = region.frames();
    for (uint32_t frame_idx = 0; frame_idx < region.frame_count(); ++frame_idx) {
        ASSERT_NE(frames + frame_idx, nullptr);
    }

    BufferFrame *frame_0 = region.allocate_frame();
    EXPECT_NE(frame_0, nullptr);
    EXPECT_EQ(frame_0, frames + 0);
    EXPECT_TRUE(region.address_in_range(frame_0));
    EXPECT_EQ(region.free_frame_count(), 29);
    region.free_frame(frame_0);
    EXPECT_EQ(region.free_frame_count(), 30);

    BufferFrame *frame_1 = region.allocate_frame();
    EXPECT_NE(frame_1, nullptr);
    EXPECT_EQ(frame_1, frames + 0);
    EXPECT_TRUE(region.address_in_range(frame_1));
    EXPECT_EQ(region.free_frame_count(), 29);
}

TEST_F(SSDDataRegionTest, WriteRead) {
    const auto page_count = 10;
    SSDRegion region{_ssd_path, page_count};
    Page test_page{};
    ASSERT_NE(test_page.data(), nullptr);
    auto page_4 = generate_random_page();
    auto page_7 = generate_random_page();
    auto null_page = Page{};
    memset(null_page.data(), 0, EFFECTIVE_PAGE_SIZE);
    // Ensure that page 4 and page 7 are never equal.
    *reinterpret_cast<uint64_t *>(page_7.data()) = 42;
    EXPECT_NE(memcmp(page_4.data(), page_7.data(), EFFECTIVE_PAGE_SIZE), 0);

    region.write_page(page_4, 4);
    region.write_page(page_7, 7);
    auto read_page_4 = Page{};
    auto read_page_7 = Page{};
    region.read_page(read_page_4, 4);
    region.read_page(read_page_7, 7);
    EXPECT_EQ(memcmp(page_4.data(), read_page_4.data(), EFFECTIVE_PAGE_SIZE), 0);
    EXPECT_EQ(memcmp(page_7.data(), read_page_7.data(), EFFECTIVE_PAGE_SIZE), 0);
    EXPECT_NE(memcmp(read_page_4.data(), read_page_7.data(), EFFECTIVE_PAGE_SIZE), 0);
}

///////////////////////////////////////////////////////////
//// Buffer Manager
///////////////////////////////////////////////////////////

class BufferManagerTest : public BasicTest {
};

TEST_F(BufferManagerTest, BasicCreate) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    EXPECT_EQ(buffer_manager->_volatile_region->frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_ssd_region->page_count(), _page_count);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count);

    BufferFrame *frame_0 = buffer_manager->allocate_page();
    ASSERT_NE(frame_0, nullptr);
    EXPECT_EQ(frame_0->page_id, 0);
    EXPECT_EQ(frame_0->parent_frame, nullptr);
    EXPECT_FALSE(frame_0->is_dirty());

    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count - 1);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 1);

    buffer_manager->free_page(frame_0);
    EXPECT_EQ(buffer_manager->_volatile_region->frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_ssd_region->page_count(), _page_count);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count);
}

TEST_F(BufferManagerTest, GetFrameHotPage) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();

    auto frame_0 = buffer_manager->allocate_page();
    ASSERT_NE(frame_0, nullptr);
    store_u64(frame_0, 13);
    auto swip = Swip(frame_0);
    auto swip_frame = swip.buffer_frame();
    EXPECT_EQ(frame_0, swip_frame);
}

TEST_F(BufferManagerTest, GetFrameColdPage) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();

    auto frame_0 = buffer_manager->allocate_page();
    ASSERT_NE(frame_0, nullptr);
    auto frame_1 = buffer_manager->allocate_page();
    ASSERT_NE(frame_1, nullptr);
    store_u64(frame_0, 20);
    store_u64(frame_1, 40);
    frame_0->mark_dirty();
    frame_1->mark_dirty();
    EXPECT_EQ(frame_0->page_id, 0);
    EXPECT_EQ(frame_1->page_id, 1);
    buffer_manager->_flush(frame_0);
    buffer_manager->_flush(frame_1);
    EXPECT_EQ(frame_0->page_id, 0);
    EXPECT_EQ(frame_1->page_id, 1);

    auto swip_0 = Swip(PageID{0});
    auto swip_1 = Swip(PageID{1});
    auto result_frame_1 = buffer_manager->get_frame(swip_1);
    auto result_frame_0 = buffer_manager->get_frame(swip_0);
    // Since more free frames are available, the buffer manager uses two new frames. Thus, the initial frames and the
    // frames in which the pages get loaded are different.
    EXPECT_NE(frame_0, result_frame_0);
    EXPECT_NE(frame_1, result_frame_1);
    EXPECT_EQ(result_frame_0->page_id, 0);
    EXPECT_EQ(result_frame_1->page_id, 1);
    EXPECT_EQ(get_u64(result_frame_0), 20);
    EXPECT_EQ(get_u64(result_frame_1), 40);
}

TEST_F(BufferManagerTest, EvictFrames) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    // As described in the LeanStore paper in Section IV E., a buffer managed data structure has to register a callback
    // to identify if a page has swips pointing to other (child) pages. In this test, a page never stores swips to another
    // page. However, besides evicting a page, `_evict` should also set the corresponding swip to evicted, i.e., store
    // the corresponding page id in the swip instead of the frame. Thus, we register a parent function that `_evict` can
    // use to get the corresponding swip for a given frame.
    auto swips = std::array<Swip, 2>{};
    swips[0] = Swip{PageID{0}};
    swips[1] = Swip{PageID{1}};
    EXPECT_FALSE(swips[0].is_swizzled() || swips[0].is_cooling());
    EXPECT_FALSE(swips[1].is_swizzled() || swips[1].is_cooling());

    auto get_parent = [&swips](BufferFrame *frame, ManagedDataStructure * /*none*/) -> Swip & {
        return swips[frame->page_id];
    };

    buffer_manager->register_callbacks({nullptr, get_parent});

    auto frame_0 = buffer_manager->allocate_page();
    auto frame_1 = buffer_manager->allocate_page();
    ASSERT_NE(frame_0, nullptr);
    ASSERT_NE(frame_1, nullptr);
    store_u64(frame_0, 20);
    store_u64(frame_1, 40);
    buffer_manager->_flush(frame_0);
    buffer_manager->_flush(frame_1);

    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 2);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count - 2);

    EXPECT_EQ(get_u64(frame_0), 20);
    EXPECT_EQ(get_u64(frame_1), 40);

    // Write new data to the pages but only set frame 0 to dirty.
    store_u64(frame_0, 300);
    store_u64(frame_1, 500);

    frame_0->mark_dirty();

    // Eviction candidates only exist if 50% of the volatile region's frames were used. Thus, at this point, no eviction
    // candidates exist.
    EXPECT_FALSE(buffer_manager->_eviction_candidate_count());
    buffer_manager->_add_eviction_candidate(frame_0);
    EXPECT_TRUE(buffer_manager->_eviction_candidate_count());
    buffer_manager->_evict_page();
    EXPECT_FALSE(buffer_manager->_eviction_candidate_count());
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 2);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count - 1);

    buffer_manager->_add_eviction_candidate(frame_1);
    EXPECT_TRUE(buffer_manager->_eviction_candidate_count());
    buffer_manager->_evict_page();
    EXPECT_FALSE(buffer_manager->_eviction_candidate_count());
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 2);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count);

    // Load pages and check content. Since frame 0 was marked as dirty before the eviction, the page was flushed. Frame 1
    // was not marked as dirty and thus was not flushed.
    frame_0 = buffer_manager->get_frame(swips[0]);
    frame_1 = buffer_manager->get_frame(swips[1]);
    EXPECT_EQ(get_u64(frame_0), 300);
    EXPECT_EQ(get_u64(frame_1), 40);
    EXPECT_TRUE(swips[0].is_swizzled());
    EXPECT_FALSE(swips[0].is_cooling());
    EXPECT_TRUE(swips[1].is_swizzled());
    EXPECT_FALSE(swips[0].is_cooling());
}

// own tests
TEST_F(BufferManagerTest, FreeAndAllocatePage) {
    std::unique_ptr<BufferManager> buffer_manager = create_default_bm();
    EXPECT_EQ(buffer_manager->_volatile_region->frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_ssd_region->page_count(), _page_count);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count);

    BufferFrame *frame_0 = buffer_manager->allocate_page();
    ASSERT_NE(frame_0, nullptr);
    EXPECT_EQ(frame_0->page_id, 0);
    EXPECT_EQ(frame_0->parent_frame, nullptr);
    EXPECT_FALSE(frame_0->is_dirty());

    BufferFrame *frame_1 = buffer_manager->allocate_page();
    ASSERT_NE(frame_1, nullptr);
    EXPECT_EQ(frame_1->page_id, 1);
    EXPECT_EQ(frame_1->parent_frame, nullptr);
    EXPECT_FALSE(frame_1->is_dirty());

    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count - 2);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 2);

    buffer_manager->free_page(frame_0);
    EXPECT_EQ(buffer_manager->_volatile_region->frame_count(), _frame_count);
    EXPECT_EQ(buffer_manager->_volatile_region->free_frame_count(), _frame_count - 1);
    EXPECT_EQ(buffer_manager->_ssd_region->page_count(), _page_count);
    EXPECT_EQ(buffer_manager->_ssd_region->free_page_count(), _page_count - 1);

    // now re allocate and check that page 0 gets back again
    BufferFrame *reallocated_frame_0 = buffer_manager->allocate_page();
    ASSERT_NE(reallocated_frame_0, nullptr);
    EXPECT_EQ(reallocated_frame_0->page_id, 0);
    EXPECT_EQ(reallocated_frame_0->parent_frame, nullptr);
    EXPECT_FALSE(reallocated_frame_0->is_dirty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
