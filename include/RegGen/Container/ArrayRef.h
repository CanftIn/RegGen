#ifndef REGGEN_CONTAINER_ARRAY_REF_H
#define REGGEN_CONTAINER_ARRAY_REF_H

#include <array>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

#include "RegGen/Container/SmallVector.h"

namespace RG {

namespace Internal {

namespace AdlDetail {

using std::begin;

template <typename RangeT>
constexpr auto begin_impl(RangeT&& range)
    -> decltype(begin(std::forward<RangeT>(range))) {
  return begin(std::forward<RangeT>(range));
}

using std::end;

template <typename RangeT>
constexpr auto end_impl(RangeT&& range)
    -> decltype(end(std::forward<RangeT>(range))) {
  return end(std::forward<RangeT>(range));
}

using std::swap;

template <typename T>
constexpr void swap_impl(T&& lhs,
                         T&& rhs) noexcept(noexcept(swap(std::declval<T>(),
                                                         std::declval<T>()))) {
  swap(std::forward<T>(lhs), std::forward<T>(rhs));
}

using std::size;

template <typename RangeT>
constexpr auto size_impl(RangeT&& range)
    -> decltype(size(std::forward<RangeT>(range))) {
  return size(std::forward<RangeT>(range));
}

}  // namespace AdlDetail

template <typename RangeT>
constexpr auto adl_begin(RangeT&& range)
    -> decltype(AdlDetail::begin_impl(std::forward<RangeT>(range))) {
  return AdlDetail::begin_impl(std::forward<RangeT>(range));
}

template <typename RangeT>
constexpr auto adl_end(RangeT&& range)
    -> decltype(AdlDetail::end_impl(std::forward<RangeT>(range))) {
  return AdlDetail::end_impl(std::forward<RangeT>(range));
}

template <typename R, typename UnaryPredicate>
auto find_if(R&& range, UnaryPredicate p) {
  return std::find_if(adl_begin(range), adl_end(range), p);
}

template <typename R, typename UnaryPredicate>
auto find_if_not(R&& range, UnaryPredicate p) {
  return std::find_if_not(adl_begin(range), adl_end(range), p);
}

}  // namespace Internal

template <typename T>
class ArrayRef {
 public:
  using value_type = T;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = const_pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iteartor = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

 private:
  const T* Data = nullptr;

  size_type Length = 0;

  void debugCheckNullptrInvariant() const {
    assert((Data != nullptr) ||
           (Length == 0) &&
               "Created ArrayRef with nullptr and non-zero length.");
  }

 public:
  ArrayRef() = default;

  ArrayRef(std::nullopt_t /*unused*/) {}

  ArrayRef(const T& one_elt) : Data(&one_elt), Length(1) {}

  constexpr ArrayRef(const T* data, size_t length)
      : Data(data), Length(length) {
    debugCheckNullptrInvariant();
  }

  constexpr ArrayRef(const T* begin, const T* end)
      : Data(begin), Length(end - begin) {
    debugCheckNullptrInvariant();
  }

  template <typename U>
  ArrayRef(const SmallVectorTemplateCommon<T, U>& vec)
      : Data(vec.data()), Length(vec.size()) {
    debugCheckNullptrInvariant();
  }

  template <
      typename Container,
      typename = std::enable_if_t<std::is_same_v<
          std::remove_const_t<decltype(std::declval<Container>().data())>, T*>>>
  ArrayRef(const Container& c) : Data(c.data()), Length(c.size()) {
    debugCheckNullptrInvariant();
  }

  template <typename A>
  ArrayRef(const std::vector<T, A>& vec)
      : Data(vec.data()), Length(vec.size()) {}

  template <size_t N>
  constexpr ArrayRef(const std::array<T, N>& arr)
      : Data(arr.data()), Length(N) {}

  template <size_t N>
  constexpr ArrayRef(const T (&arr)[N]) : Data(arr), Length(N) {}

  constexpr ArrayRef(const std::initializer_list<T>& vec)
      : Data(vec.begin() == vec.end() ? static_cast<T*>(nullptr) : vec.begin()),
        Length(vec.size()) {}

  template <typename U>
  ArrayRef(
      const ArrayRef<U*>& a,
      std::enable_if_t<std::is_convertible_v<U* const*, T const*>>* /*unused*/ =
          nullptr)
      : Data(a.data()), Length(a.size()) {}

  template <typename U, typename DummyT>
  ArrayRef(
      const SmallVectorTemplateCommon<U*, DummyT>& vec,
      std::enable_if_t<std::is_convertible_v<U* const*, T const*>>* /*unused*/ =
          nullptr)
      : Data(vec.data()), Length(vec.size()) {}

  template <typename U, typename A>
  ArrayRef(
      const std::vector<U*, A>& vec,
      std::enable_if_t<std::is_convertible_v<U* const*, T const*>>* /*unused*/ =
          nullptr)
      : Data(vec.data()), Length(vec.size()) {}

  auto begin() const -> iterator { return Data; }
  auto end() const -> iterator { return Data + Length; }

  constexpr auto cbegin() const -> const_iterator { return Data; }
  constexpr auto cend() const -> const_iterator { return Data + Length; }

  auto rbegin() const -> reverse_iterator { return reverse_iterator(end()); }
  auto rend() const -> reverse_iterator { return reverse_iterator(begin()); }

  auto crbegin() const -> const_reverse_iteartor {
    return const_reverse_iteartor(cend());
  }
  auto crend() const -> const_reverse_iteartor {
    return const_reverse_iteartor(cbegin());
  }

  constexpr auto empty() const -> bool { return Length == 0; }
  constexpr auto data() const -> const T* { return Data; }
  constexpr auto size() const -> size_t { return Length; }

  auto front() const -> const T& {
    assert(!empty() && "Cannot get the front of an empty ArrayRef");
    return Data[0];
  }

  auto back() const -> const T& {
    assert(!empty() && "Cannot get the back of an empty ArrayRef");
    return Data[Length - 1];
  }

  constexpr auto equals(ArrayRef rhs) const -> bool {
    return Length == rhs.Length && std::equal(begin(), end(), rhs.begin());
  }

  auto slice(size_t n, size_t m) const -> ArrayRef<T> {
    assert(n + m <= size() && "Invalid slice range");
    return ArrayRef<T>(data() + n, m);
  }

  auto slice(size_t n) const -> ArrayRef<T> { return slice(n, size() - n); }

  auto drop_front(size_t n = 1) const -> ArrayRef<T> {
    return slice(n, size() - n);
  }

  auto drop_back(size_t n = 1) const -> ArrayRef<T> {
    return slice(0, size() - n);
  }

  template <class PredicateT>
  auto drop_while(PredicateT pred) const -> ArrayRef<T> {
    return ArrayRef<T>(Internal::find_if_not(*this, pred), end());
  }

  template <class PredicateT>
  auto drop_until(PredicateT pred) const -> ArrayRef<T> {
    return ArrayRef<T>(Internal::find_if(*this, pred), end());
  }

  auto take_front(size_t n = 1) const -> ArrayRef<T> {
    if (n >= size()) {
      return *this;
    }
    return drop_back(size() - n);
  }

  auto take_back(size_t n = 1) const -> ArrayRef<T> {
    if (n >= size()) {
      return *this;
    }
    return drop_front(size() - n);
  }

  template <class PredicateT>
  auto take_while(PredicateT pred) const -> ArrayRef<T> {
    return ArrayRef<T>(begin(), Internal::find_if_not(*this, pred));
  }

  template <class PredicateT>
  auto take_until(PredicateT pred) const -> ArrayRef<T> {
    return ArrayRef<T>(begin(), Internal::find_if(*this, pred));
  }

  constexpr auto operator[](size_t index) const -> const T& {
    assert(index < Length && "Invalid index!");
    return Data[index];
  }

  /// Disallow accidental assignment from a temporary.
  ///
  /// The declaration here is extra complicated so that "arrayRef = {}"
  /// continues to select the move assignment operator.
  template <typename U>
  auto operator=(U&& temporary)
      -> std::enable_if_t<std::is_same_v<U, T>, ArrayRef<T>>& = delete;

  template <typename U>
  auto operator=(std::initializer_list<U>)
      -> std::enable_if_t<std::is_same_v<U, T>, ArrayRef<T>>& = delete;

  auto vec() const -> std::vector<T> {
    return std::vector<T>(Data, Data + Length);
  }

  operator std::vector<T>() const { return vec(); }
};

template <typename T>
ArrayRef(const T& one_elt) -> ArrayRef<T>;

template <typename T>
ArrayRef(const T* data, size_t length) -> ArrayRef<T>;

template <typename T>
ArrayRef(const T* data, const T* end) -> ArrayRef<T>;

template <typename T>
ArrayRef(const SmallVectorImpl<T>& vec) -> ArrayRef<T>;

template <typename T, unsigned N>
ArrayRef(const SmallVector<T, N>& vec) -> ArrayRef<T>;

template <typename T>
ArrayRef(const std::vector<T>& vec) -> ArrayRef<T>;

template <typename T, std::size_t N>
ArrayRef(const std::array<T, N>& vec) -> ArrayRef<T>;

template <typename T>
ArrayRef(const ArrayRef<T>& vec) -> ArrayRef<T>;

template <typename T>
ArrayRef(ArrayRef<T>& vec) -> ArrayRef<T>;

template <typename T, size_t N>
ArrayRef(const T (&arr)[N]) -> ArrayRef<T>;

template <typename T>
auto makeArrayRef(const T& one_elt) -> ArrayRef<T> {
  return one_elt;
}

template <typename T>
auto makeArrayRef(const T* data, size_t length) -> ArrayRef<T> {
  return ArrayRef<T>(data, length);
}

template <typename T>
auto makeArrayRef(const T* begin, const T* end) -> ArrayRef<T> {
  return ArrayRef<T>(begin, end);
}

template <typename T>
auto makeArrayRef(const SmallVectorImpl<T>& vec) -> ArrayRef<T> {
  return vec;
}

template <typename T, unsigned N>
auto makeArrayRef(const SmallVector<T, N>& vec) -> ArrayRef<T> {
  return vec;
}

template <typename T>
auto makeArrayRef(const std::vector<T>& vec) -> ArrayRef<T> {
  return vec;
}

template <typename T, std::size_t N>
auto makeArrayRef(const std::array<T, N>& arr) -> ArrayRef<T> {
  return arr;
}

template <typename T>
auto makeArrayRef(const ArrayRef<T>& vec) -> ArrayRef<T> {
  return vec;
}

template <typename T>
auto makeArrayRef(ArrayRef<T>& vec) -> ArrayRef<T>& {
  return vec;
}

template <typename T, size_t N>
auto makeArrayRef(const T (&arr)[N]) -> ArrayRef<T> {
  return ArrayRef<T>(arr);
}

template <typename T>
auto operator==(ArrayRef<T> a1, ArrayRef<T> a2) -> bool {
  return a1.equals(a2);
}

template <typename T>
auto operator!=(ArrayRef<T> a1, ArrayRef<T> a2) -> bool {
  return !a1.equals(a2);
}

template <typename T>
auto operator==(const std::vector<T>& a1, ArrayRef<T> a2) -> bool {
  return ArrayRef<T>(a1).equals(a2);
}

template <typename T>
auto operator!=(const std::vector<T>& a1, ArrayRef<T> a2) -> bool {
  return !ArrayRef<T>(a1).equals(a2);
}

template <typename T>
auto operator==(ArrayRef<T> a1, const std::vector<T>& a2) -> bool {
  return a1.equals(ArrayRef<T>(a2));
}

template <typename T>
auto operator!=(ArrayRef<T> a1, const std::vector<T>& a2) -> bool {
  return !a1.equals(ArrayRef<T>(a2));
}

}  // namespace RG

#endif  // REGGEN_CONTAINER_ARRAY_REF_H