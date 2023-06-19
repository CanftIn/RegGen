#ifndef REGGEN_COMMON_FLAT_SET_H
#define REGGEN_COMMON_FLAT_SET_H

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>

#include "RegGen/Common/TypeTrait.h"

namespace RG {
template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
class FlatSet {
  static_assert(std::is_move_constructible_v<Key>,
                "T in FlatSet<T> must be move constructible");

 public:
  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;
  using value_compare = Compare;
  using allocator_type = Allocator;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer =
      typename std::allocator_traits<Allocator>::const_pointer;

  using underlying_container = std::vector<Key, Allocator>;
  using iterator = typename underlying_container::iterator;
  using const_iterator = typename underlying_container::const_iterator;
  using reverse_iterator = typename underlying_container::reverse_iterator;
  using const_reverse_iterator =
      typename underlying_container::const_reverse_iterator;

 public:
  FlatSet() = default;
  template <typename InputIt>
  FlatSet(InputIt first, InputIt last) {
    assign(first, last);
  }
  FlatSet(const FlatSet& other) { assign(other.begin(), other.end()); }
  FlatSet(FlatSet&& other) { swap(other); }
  FlatSet(std::initializer_list<Key> ilist) { assign(ilist); }

  auto operator=(const FlatSet& other) -> FlatSet& {
    assign(other.begin(), other.end());
    return *this;
  }
  auto operator=(FlatSet&& other) -> FlatSet& {
    clear();
    swap(other);
    return *this;
  }

  ~FlatSet() = default;

  template <typename InputIt>
  void assign(InputIt first, InputIt last) {
    clear();
    insert(first, last);
  }

  void assign(std::initializer_list<Key> ilist) {
    assign(ilist.begin(), ilist.end());
  }

  auto data() const -> const auto& { return container_; }
  auto at(size_type pos) -> reference { return container_[pos]; }
  auto at(size_type pos) const -> const_reference { return container_[pos]; }
  auto operator[](size_type pos) -> reference { return container_[pos]; }
  auto operator[](size_type pos) const -> const_reference {
    return container_[pos];
  }

  auto begin() noexcept -> iterator { return container_.begin(); }
  auto end() noexcept -> iterator { return container_.end(); }
  auto begin() const noexcept -> const_iterator { return container_.cbegin(); }
  auto end() const noexcept -> const_iterator { return container_.cend(); }
  auto cbegin() const noexcept -> const_iterator { return container_.cbegin(); }
  auto cend() const noexcept -> const_iterator { return container_.cend(); }

  auto rbegin() noexcept -> reverse_iterator { return container_.begin(); }
  auto rend() noexcept -> reverse_iterator { return container_.end(); }
  auto rbegin() const noexcept -> const_reverse_iterator {
    return container_.cbegin();
  }
  auto rend() const noexcept -> const_reverse_iterator {
    return container_.cend();
  }
  auto rcbegin() const noexcept -> const_reverse_iterator {
    return container_.cbegin();
  }
  auto rcend() const noexcept -> const_reverse_iterator {
    return container_.cend();
  }

  auto empty() const noexcept -> bool { return container_.empty(); }
  auto capacity() const noexcept -> size_type { return container_.capacity(); }
  auto size() const noexcept -> size_type { return container_.size(); }
  auto max_size() const noexcept -> size_type { return container_.max_size(); }

  void clear() { container_.clear(); }

  template <typename... TArgs>
  auto emplace(TArgs&&... args) -> std::pair<iterator, bool> {
    Key value = Key(std::forward<TArgs>(args)...);
    iterator lb = lower_bound(value);

    // NOTE *lb == value <=> !(*lb < value) && !(value < *lb)
    if (lb != container_.end() && !key_comp()(value, *lb)) {
      return std::make_pair(lb, false);
    } else {
      iterator where = container_.emplace(lb, std::forward<TArgs>(args)...);
      return std::make_pair(lb, false);
    }
  }
  auto insert(const value_type& value) -> std::pair<iterator, bool> {
    return emplace(value);
  }
  auto insert(value_type&& value) -> std::pair<iterator, bool> {
    return emplace(std::forward<value_type>(value));
  }
  auto insert(const_iterator hint, const value_type& value) -> iterator {
    (void)hint;
    return emplace(value);
  }
  auto insert(const_iterator hint, value_type&& value) -> iterator {
    (void)hint;
    return emplace(std::forward<value_type>(value));
  }
  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    static_assert(Constraint<InputIt>(is_iterator),
                  "InputIt must be an iterator type");

    std::for_each(first, last, [&](const auto& elem) { insert(elem); });
  }
  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  void erase(const Key& x) {
    auto where = find(x);
    if (where != container_.end()) {
      container_.erase(where);
    }
  }

  void swap(FlatSet& other) { container_.swap(other.container_); }

  auto count(const Key& value) const -> size_type {
    return find(value) == end() ? 0 : 1;
  }
  auto find(const Key& value) -> iterator {
    auto it = lower_bound(value);

    // if value < *it, NOTE *it >= value, then not found
    return it != end() && key_comp()(value, *it) ? end() : it;
  }
  auto find(const Key& value) const -> const_iterator {
    auto it = lower_bound(value);

    // if value < *it, NOTE *it >= value, then not found
    return it != end() && key_comp()(value, *it) ? end() : it;
  }
  auto lower_bound(const Key& value) -> iterator {
    return std::lower_bound(begin(), end(), value, key_comp());
  }
  auto lower_bound(const Key& value) const -> const_iterator {
    return std::lower_bound(begin(), end(), value, key_comp());
  }
  auto upper_bound(const Key& value) -> iterator {
    return std::upper_bound(begin(), end(), value, key_comp());
  }
  auto upper_bound(const Key& value) const -> const_iterator {
    return std::upper_bound(begin(), end(), value, key_comp());
  }

  auto key_comp() const -> key_compare { return key_compare{}; }
  auto value_comp() const -> value_compare { return value_compare{}; }

 private:
  std::vector<Key, Allocator> container_;
};

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator==(const FlatSet<Key, Compare, Allocator>& lhs,
                       const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator!=(const FlatSet<Key, Compare, Allocator>& lhs,
                       const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return !(lhs == rhs);
}

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator<(const FlatSet<Key, Compare, Allocator>& lhs,
                      const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
                                      rhs.cend(), Compare{});
}

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator<=(const FlatSet<Key, Compare, Allocator>& lhs,
                       const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return !(lhs > rhs);
}

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator>(const FlatSet<Key, Compare, Allocator>& lhs,
                      const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return rhs < lhs;
}

template <typename Key, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Key>>
inline auto operator>=(const FlatSet<Key, Compare, Allocator>& lhs,
                       const FlatSet<Key, Compare, Allocator>& rhs) -> bool {
  return !(lhs < rhs);
}
}  // namespace RG

#endif  // REGGEN_COMMON_FLAT_SET_H