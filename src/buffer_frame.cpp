#include "buffer_frame.hpp"

std::byte* Page::data() {
  // TODO(student) implement
  return nullptr;
}

Page::operator std::byte*() { return reinterpret_cast<std::byte*>(this); }

void BufferFrame::mark_dirty() {
  // TODO(student) implement
}

void BufferFrame::mark_written_back() {
  // TODO(student) implement
}

bool BufferFrame::is_dirty() {
  // TODO(student) implement
  return true;
}
