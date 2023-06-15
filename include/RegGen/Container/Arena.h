#ifndef REGGEX_CONTAINER_ARENA_H
#define REGGEX_CONTAINER_ARENA_H

#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <type_traits>

#include "RegGen/Common/InheritRestrict.h"

namespace RG {

class Arena final : NonCopyable, NonMovable {
 private:
  struct Block {
    Block* next;
    size_t size;     // size of the block
    size_t offset;   // available space in the block
    size_t counter;  // number of allocations failure in the block
    auto DataAddress() -> char* { return reinterpret_cast<char*>(this + 1); }
  };

  struct DestructorHandle {
    void* pointer;
    void (*cleaner)(void*);
  };

  static constexpr size_t DefaultAlignment = alignof(std::nullptr_t);
  static constexpr size_t FailureToleranceCount = 8;
  static constexpr size_t FailureCounterThreshold = 1024;
  static constexpr size_t BigChunkThreshold = 2048;
  static constexpr size_t DefaultPoolBlockSize = 4096 - sizeof(Block);
  static constexpr size_t MaximumPoolBlockSize =
      static_cast<unsigned>(16 * 4096) - sizeof(Block);
  static constexpr float PoolBlockGrowthFactor = 2;

 public:
  Arena() = default;

  ~Arena() {
    for (auto handle : destructors_) {
      handle.cleaner(handle.pointer);
    }
    FreeBlocks(pooled_head_);
    FreeBlocks(big_node_);
  }

  static auto Create() -> std::unique_ptr<Arena> {
    return std::make_unique<Arena>();
  }

  static auto CreateShared() -> std::shared_ptr<Arena> {
    return std::make_shared<Arena>();
  }

  auto Allocate(size_t sz) -> void*;

  template <typename T, typename... Args>
  auto Construct(Args&&... args) -> T* {
    auto* ptr = Allocate(sizeof(T));
    new (ptr) T(std::forward<Args>(args)...);

    if constexpr (!std::is_trivially_destructible_v<T>) {
      destructors_.push_back(
          {ptr, [](void* p) { reinterpret_cast<T*>(p)->~T(); }});
    }

    return reinterpret_cast<T*>(ptr);
  }

  auto GetByteAllocated() const -> size_t {
    return CalculateUsage(pooled_head_, false) +
           CalculateUsage(big_node_, false);
  }

  auto GetByteUsed() const -> size_t {
    return CalculateUsage(pooled_head_, true) + CalculateUsage(big_node_, true);
  }

 private:
  auto NewBlock(size_t capacity) -> Block*;

  auto NewPoolBlock() -> Block*;

  auto FreeBlocks(Block* list) const -> void;

  auto CalculateUsage(Block* list, bool used) const -> size_t;

  auto AllocSmallChunk(size_t sz) -> void*;

  auto AllocBigChunk(size_t sz) -> void*;

  size_t next_block_size_ = DefaultPoolBlockSize;
  Block* pooled_head_ = nullptr;
  Block* pooled_current_ = nullptr;
  Block* big_node_ = nullptr;

  std::deque<DestructorHandle> destructors_;
};

}  // namespace RG

#endif  // REGGEX_CONTAINER_ARENA_H