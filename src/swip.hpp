#pragma once

#include "buffer_frame.hpp"

// A swip is a reference to a page. It either stores a pointer to a buffer frame storing the referenced page in memory
// or a page id. A swip can be (1) swizzled, i.e., it stores a valid buffer frame address, (2) unswizzled/cooling, i.e.,
// it stores a pointer to a buffer frame but one or multiple bits are flipped to indicate that the referenced frame is
// unswizzled/cooling. Alternatively, a swip can be (3) unswizzled/evicted, i.e., it does not store a valid frame
// address but a page id instead. Similar to the unswizzeld/cooling state, individual bits might be used to indicate
// this swip state. You can use the two least significant bits for indicating the states / pointer tagging.

class Swip {
 public:
  // Creates an unswizzled swip with an invalid page id. The swip indicates that the referenced page is not swizzled,
  // not cooling, but evicted.
  Swip();

  // Creates an unswizzled swip storing the given page id. The swip indicates that the referenced page is not swizzled,
  // not cooling, but evicted.
  Swip(PageID page_id);

  // Creates a swizzled swip storing the buffer_frame. The swip indicates that the referenced page is swizzled,
  // not cooling, not evicted.
  Swip(BufferFrame* buffer_frame);

  // Returns whether the swip is swizzled.
  bool is_swizzled();

  // Returns whether the swip is cooling.
  bool is_cooling();

  // Returns whether the swip is evicted.
  bool is_evicted();

  // Assumes that the frame pointer is still stored and valid but unswizzled/cooling. Thus, cooling bit(s) are set
  // accordingly. Flips the cooling bit(s) so that the stored frame pointer address is correct (swip is swizzled).
  void swizzle();

  // Stores the given frame pointer address. Swip is swizzled afterwards.
  void swizzle(BufferFrame* buffer_frame);

  // Flips the cooling bit(s) to indicate that the swip is in cooling state. Assumes that the swip is swizzled.
  void unswizzle();

  // Sets the page ID and one or more bits indicating that this swip is unswizzled/evicted.
  void evict(PageID page_id);

  // Returns the stored page id. Note that the two least significant bits are used for pointer tagging.
  PageID page_id();

  // Returns the stored buffer frame. This expects that the frame is stored in the swip, i.e., the swip is swizzled and
  // it is not in cooling state.
  BufferFrame* buffer_frame();

  // Returns the stored buffer frame. This expects the swip to be (1) swizzled or (2) cooling. To get a valid pointer
  // address, the bit(s) used for the cooling state have to be ignored. The state of the swip does not change.
  BufferFrame* buffer_frame_ignore_tags();

 private:
  // Note, that a swip stores either a buffer frame pointer or a page id. The size of a swip thus should be 8 Byte.
  // You might want do define constants here for bit modifications and comparisons.
  // -- TODO(student)
};
