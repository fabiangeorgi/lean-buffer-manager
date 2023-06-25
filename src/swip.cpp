#include "swip.hpp"

#include "buffer_frame.hpp"

Swip::Swip() {
  // TODO(student) implement
}

Swip::Swip(PageID page_id) {
  // TODO(student) implement
}

Swip::Swip(BufferFrame* buffer_frame) {
  // TODO(student) implement
}

bool Swip::is_swizzled() {
  // TODO(student) implement
  return false;
};

bool Swip::is_cooling() {
  // TODO(student) implement
  return false;
}

bool Swip::is_evicted() {
  // TODO(student) implement
  return false;
}

void Swip::swizzle() {
  // TODO(student) implement
}

void Swip::swizzle(BufferFrame* buffer_frame) {
  // TODO(student) implement
}

void Swip::unswizzle() {
  // TODO(student) implement
}

void Swip::evict(PageID page_id) {
  // TODO(student) implement
}

PageID Swip::page_id() {
  // TODO(student) implement
  return 0ul;
}

BufferFrame* Swip::buffer_frame() {
  // TODO(student) implement
  return nullptr;
}

BufferFrame* Swip::buffer_frame_ignore_tags() {
  // TODO(student) implement
  return nullptr;
}
