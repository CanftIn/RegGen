#ifndef REGGEN_CONTAINER_HEAP_ARRAY_H
#define REGGEN_CONTAINER_HEAP_ARRAY_H

#include <cassert>
#include <memory>
#include <type_traits>

#include "RegGen/Common/InheritRestrict.h"
#include "RegGen/Container/ArrayRef.h"

namespace RG {

template <typename T>
class HeapArray : NonCopyable {
 public:
  static_assert(!std::is_const_v<T> || !std::is_volatile_v<T>,
                "T cannot be cv-qualified");

  HeapArray() = default;
  HeapArray(int len) { initialize(len); }
  HeapArray(int len, const T& value) { initialize(len, value); }

  HeapArray(HeapArray&& other) { this->swap(other); }
  auto operator=(HeapArray&& other) -> HeapArray& {
    initialize(0);
    this->swap(other);
  }

  ~HeapArray() { Destroy(); }

  auto empty() const noexcept -> bool { return size_ == 0; }

  auto size() const noexcept -> int { return size_; }

  auto front() -> T& { return at(0); }
  auto front() const -> const T& { return at(0); }

  auto back() -> T& { return at(size_ - 1); }
  auto back() const -> const T& { return at(size_ - 1); }

  auto at(int index) -> T& {
    assert(index >= 0 && index < size_);
    return ptr_[index];
  }
  auto at(int index) const -> const T& {
    assert(index >= 0 && index < size_);
    return ptr_[index];
  }

  auto ref() -> ArrayRef<T> { return ArrayRef<T>{ptr_, size_}; }

  auto initialize(int len) -> void {
    InitializeInternal(
        len, [&](T* p) { std::uninitialized_value_construct_n(p, len); });
  }

  auto initialize(int len, const T& value) -> void {
    InitializeInternal(len,
                       [&](T* p) { std::uninitialized_fill_n(p, len, value); });
  }

  auto operator[](int index) -> T& { return at(index); }
  auto operator[](int index) const -> const T& { return at(index); }

  auto begin() -> T* { return ptr_; }
  auto begin() const noexcept -> const T* { return ptr_; }

  auto end() -> T* { return ptr_ + size_; }
  auto end() const noexcept -> const T* { return ptr_ + size_; }

  auto swap(HeapArray& other) noexcept -> void {
    std::swap(size_, other.size_);
    std::swap(ptr_, other.ptr_);
  }

 private:
  auto Destroy() noexcept -> void {
    if (ptr_) {
      std::destroy_n(ptr_, size_);
    }

    free(ptr_);
    size_ = 0;
    ptr_ = nullptr;
  }

  template <typename FInit>
  auto InitializeInternal(int len, FInit init) -> void {
    assert(len >= 0);

    auto new_size = 0;
    T* new_ptr = nullptr;

    if (len > 0) {
      auto tmp = reinterpret_cast<T*>(malloc(sizeof(T) * len));

      try {
        init(tmp);
        new_size = len;
        new_ptr = tmp;
      } catch (...) {
        free(tmp);
        throw;
      }
    }

    Destroy();
    size_ = new_size;
    ptr_ = new_ptr;
  }

  int size_ = 0;
  T* ptr_ = nullptr;
};

template <typename T>
inline auto operator==(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return lhs.Size() == rhs.Size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
template <typename T>
inline auto operator!=(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return !(lhs == rhs);
}
template <typename T>
inline auto operator<(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                      rhs.end());
}
template <typename T>
inline auto operator>(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return rhs < lhs;
}
template <typename T>
inline auto operator<=(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return !(lhs > rhs);
}
template <typename T>
inline auto operator>=(const HeapArray<T>& lhs, const HeapArray<T>& rhs) -> bool {
  return !(lhs < rhs);
}

}  // namespace RG

#endif  // REGGEN_CONTAINER_HEAP_ARRAY_H