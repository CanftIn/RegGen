#include "RegGen/Common/Arena.h"

#include <cstdlib>

namespace RG {

auto Arena::Allocate(size_t sz) -> void* {
  if (sz % DefaultAlignment) {
    sz += DefaultAlignment - sz % DefaultAlignment;
  }

  if (sz > BigChunkThreshold) {
    return AllocBigChunk(sz);
  } else {
    return AllocSmallChunk(sz);
  }
}

auto Arena::NewBlock(size_t capacity) -> Block* {
  void* p = malloc(sizeof(Block) + capacity);
  auto* block = reinterpret_cast<Block*>(p);

  block->next = nullptr;
  block->size = capacity;
  block->offset = 0;
  block->counter = 0;

  return block;
}

auto Arena::NewPoolBlock() -> Block* {
  auto size = next_block_size_;
  auto* block = NewBlock(size);
  next_block_size_ = std::min(static_cast<size_t>(PoolBlockGrowthFactor * size),
                              MaximumPoolBlockSize);

  return block;
}

auto Arena::FreeBlocks(Block* list) const -> void {
  Block* p = list;
  while (p != nullptr) {
    auto* next = p->next;
    free(p);
    p = next;
  }
}

auto Arena::CalculateUsage(Block* list, bool used) const -> size_t {
  size_t sum = 0;
  for (Block* p = list; p != nullptr; p = p->next) {
    if (used) {
      sum += p->offset;
    } else {
      sum += p->size;
    }
  }
  return sum;
}

auto Arena::AllocSmallChunk(size_t sz) -> void* {
  // lazy initialization
  if (pooled_current_ == nullptr) {
    pooled_head_ = pooled_current_ = NewPoolBlock();
  }

  // find a chunk of memory in the memory pool
  Block* cur = pooled_current_;
  while (true) {
    size_t available_sz = cur->size - cur->offset;
    if (available_sz >= sz) {
      // enough memory in the current Block
      void* addr = cur->DataAddress() + cur->offset;
      cur->offset += sz;
      return addr;
    } else {
      if (available_sz < FailureCounterThreshold) {
        // failed to allocate and available size is small enough
        cur->counter += 1;
      }

      // roll to next Block, allocate one if neccessary
      Block* next =
          cur->next != nullptr ? cur->next : (cur->next = NewPoolBlock());
      // if the current Block has beening failing for too many times, drop it
      if (cur->counter > FailureToleranceCount) {
        pooled_current_ = next;
      }

      cur = next;
    }
  }
}

auto Arena::AllocBigChunk(size_t sz) -> void* {
  Block* cur = NewBlock(sz);
  cur->next = big_node_;
  big_node_ = cur;

  return cur->DataAddress();
}

}  // namespace RG