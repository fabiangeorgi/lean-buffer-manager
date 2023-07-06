#pragma once

#include <functional>
#include <list>
#include <memory>
#include <random>
#include <unordered_map>

#include "buffer_frame.hpp"
#include "data_regions.hpp"
#include "swip.hpp"

// Share of pages in cooling stage. Do not modify.
constexpr float SHARE_COOLING_PAGES = 0.1f;
// Share of the free frames that has to be achieved so that the number of eviction candidates gets maintained.
// We round down, i.e., static_cast<some uint type>(frame_count * SHARE_USED_PAGES_BEFORE_COOLING) should be
// sufficient to calculate the number of used candidates.
constexpr float SHARE_USED_PAGES_BEFORE_COOLING = 0.5f;

// Base class for all concrete data structures that can be managed by the buffer manager.
struct ManagedDataStructure {};

// ----- Callback functions (see Paper IV. E.)
// Callback function to iterate over a page's child swips. This is necessary to check if a randomly picked page has
// swizzled child pages. If a swizzled child swip is found, set `frame` to the swizzled child's frame and return true.
// If no swizzled child was found, frame does not get changed and the function returns false.
using Functor = std::function<bool(Swip&)>;
using IterateChildSwipsFunction = std::function<bool(BufferFrame* frame, Functor)>;

// Callback function to get the parent swip of a given frame.
using GetParentFunction = std::function<Swip&(BufferFrame* frame, ManagedDataStructure* data_structure)>;

struct Callbacks {
  IterateChildSwipsFunction iterate_children = nullptr;
  GetParentFunction get_parent = nullptr;
};

class BufferManager {
 public:
  BufferManager(std::unique_ptr<VolatileRegion> volatile_region, std::unique_ptr<SSDRegion> ssd_region);

  // Allocates a new frame with the next available page id. For the tests and benchmark, you can assume than the SSD
  // region has enough pages. If you run into an issue here, you are probably allocating too many page IDs.
  //
  // If no more frames are available, another frame needs to be evicted. To evict a page, we need a certain number of
  // eviction candidates, i.e., frames in the cooling stage. This task is single-threaded and the number of eviction
  // candidates needs to be ensured synchronously. We ensure the number of candidates only when we have already
  // allocated 50% of the available frames. We ensure the number of eviction candidates when (1) a new page is
  // allocated, or (2) a cold page needs to be loaded into a frame and thus a free frame is required. In this function,
  // we ensure the number of eviction candidates after allocating the frame.
  BufferFrame* allocate_page();

  // Frees the frame and the corresponding page id.
  void free_page(BufferFrame* frame);

  // Depending on the state of a swip state, various things can happen in this method. (1) If the swip is swizzled, the
  // stored buffer frame can simply be returned. (2) If the swip is unswizzled/cooling, it needs to be swizzled by
  // flipping the cooling bit(s). Note that swizzling a cooling swip also removes the related frame from the set of
  // eviction candidates. (3) If the swip is unswizzled/evicted, the corresponding page needs to be loaded from disk and
  // the swip needs to be swizzled with a buffer frame storing the read page data. In any case, this function returns a
  // frame holding the requested page data.
  //
  // Similar to `allocate_page`, we need to ensure that one frame is available if the page to be retrieved by resolving
  // the swip is evicted. In this case, this function has to ensure the required number of eviction candidates after
  // allocating a frame for the page to be loaded. Feel free to add more helper functions to avoid writing redundant
  // code.
  BufferFrame* get_frame(Swip& swip);

  // Register the callback functions.
  void register_callbacks(Callbacks&& callbacks);

  // Registers a data structure. This might be relevant for a concrete data structure's callback functions.
  void register_data_structure(ManagedDataStructure* data_structure);

  // --- The below variables and functions do not necessarily need to be public. However, this makes testing much
  // easier.

  // Flushed the page of the passed buffer frame.
  void _flush(BufferFrame* frame);

  // Evicts a page. The cooling stage to be implemented determines which page to evict.
  void _evict_page();

  // Checks if the passed buffer frame is an eviction candidate. In terms of the second chance lean eviction policy
  // described in the paper, this function checks if the frame is in the cooling stage.
  bool _has_eviction_candidate(BufferFrame* frame);

  // Pops and returns the frame that is to be evicted.
  BufferFrame* _pop_eviction_candidate();

  // Adds the passed frame to the set of eviction candidates. In terms of the second chance eviction policy, this
  // function adds the frame to the cooling stage.
  void _add_eviction_candidate(BufferFrame* frame);

  // Removes the passed frame from the set of eviction candidates (if it is present).
  void _remove_eviction_candidate(BufferFrame* frame);

  // Returns the number of frames in the set of eviction candidates.
  uint32_t _eviction_candidate_count();

  std::unique_ptr<VolatileRegion> _volatile_region;
  std::unique_ptr<SSDRegion> _ssd_region;
  Callbacks _callbacks;
  ManagedDataStructure* _managed_data_structure;

  // Random number generation
  std::mt19937 _random_generator;
  std::uniform_int_distribution<uint64_t> _distribution;

 private:
  // Chooses a random frame. Do not modify the code.
  BufferFrame* _random_frame();

  // IDEA: https://stackoverflow.com/questions/8619404/hash-list-with-stl-can-i-point-to-an-item-in-an-stl-list-to-another-item/8619656#8619656
  std::unordered_map<BufferFrame*, std::list<BufferFrame*>::iterator> fast_access = {};
  std::list<BufferFrame*> eviction_list = {};

  void _create_cooling_state_share(BufferFrame* bf);
};
