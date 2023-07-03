#include "buffer_frame.hpp"

std::byte* Page::data() {
    return this->payload.begin();
}

Page::operator std::byte*() { return reinterpret_cast<std::byte*>(this); }

void BufferFrame::mark_dirty() {
  dirty = true;
}

void BufferFrame::mark_written_back() {
  dirty = false;
}

bool BufferFrame::is_dirty() {
  return dirty;
}
