#ifndef REGGEN_CONTAINER_ARRAY_H_
#define REGGEN_CONTAINER_ARRAY_H_

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

namespace RG {

namespace Internal {

template <typename Tp, size_t N>
struct ArrayTraits final {
  using Type = Tp[N];

  static constexpr auto ref(const Type& t, size_t n) noexcept -> Tp& {
    return const_cast<Tp&>(t[n]);
  }

  static constexpr auto ptr(const Type& t) noexcept -> Tp* {
    return const_cast<Tp*>(t);
  }
};

template <typename Tp>
struct ArrayTraits<Tp, 0> final {
  struct Type final {};

  static constexpr auto ref(const Type& t, std::size_t /*unused*/) noexcept
      -> Tp& {
    return *ptr(t);
  }

  static constexpr auto ptr(const Type& /*unused*/) noexcept -> Tp* {
    return nullptr;
  }
};

}  // namespace Internal

template <typename Tp, size_t N>
class Array final {
 public:
  using value_type = Tp;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  using ArrType = Internal::ArrayTraits<Tp, N>;

 public:
  typename ArrType::Type Elems;

  constexpr void fill(const value_type& v) { std::fill_n(begin(), size(), v); }

  template <size_t M, std::enable_if_t<(M <= N), int> = 0>
  void swap(Array<Tp, M>& other) noexcept {
    std::swap_ranges(begin(), begin() + M, other.begin());
  }

  auto begin() noexcept -> iterator { return iterator(data()); }

  constexpr auto begin() const noexcept -> const_iterator {
    return const_iterator(data());
  }

  auto end() noexcept -> iterator { return iterator(data() + size()); }

  constexpr auto end() const noexcept -> const_iterator {
    return const_iterator(data() + size());
  }

  constexpr auto rbegin() noexcept -> reverse_iterator {
    return reverse_iterator(end());
  }

  constexpr auto rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(end());
  }

  constexpr auto rend() noexcept -> reverse_iterator {
    return reverse_iterator(begin());
  }

  constexpr auto rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(begin());
  }

  constexpr auto cbegin() const noexcept -> const_iterator {
    return const_iterator(data());
  }

  constexpr auto cend() const noexcept -> const_iterator {
    return const_iterator(data() + size());
  }

  constexpr auto crbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(end());
  }

  constexpr auto crend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(begin());
  }

  constexpr auto size() const noexcept -> size_type { return N; }

  constexpr auto max_size() const noexcept -> size_type { return N; }

  constexpr auto empty() const noexcept -> bool { return N == 0; }

  constexpr auto operator[](size_type n) noexcept -> reference {
    return ArrType::ref(Elems, n);
  }

  constexpr auto operator[](size_type n) const noexcept -> const_reference {
    return ArrType::ref(Elems, n);
  }

  auto at(size_type n) -> reference {
    if (n >= size()) {
      throw std::out_of_range("Array::at");
    }
    return ArrType::ref(Elems, n);
  }

  constexpr auto at(size_type n) const -> const_reference {
    return n < N ? ArrType::ref(Elems, n)
                 : throw std::out_of_range("Array::at"),
           ArrType::ref(Elems, n);
  }

  auto front() noexcept -> reference { return *begin(); }

  constexpr auto front() const noexcept -> const_reference { return *begin(); }

  auto back() noexcept -> reference { return N ? *(end() - 1) : *end(); }

  constexpr auto back() const noexcept -> const_reference {
    return N ? ArrType::ref(Elems, N - 1) : ArrType::ref(Elems, N);
  }

  auto data() noexcept -> pointer { return ArrType::ptr(Elems); }

  constexpr auto data() const noexcept -> const_pointer {
    return ArrType::ptr(Elems);
  }
};

template <typename Tp, typename... Up>
Array(Tp, Up...) -> Array<std::enable_if_t<(std::is_same_v<Tp, Up> && ...), Tp>,
                          1 + sizeof...(Up)>;

namespace Internal {

template <class T, size_t N>
constexpr inline auto array_equals(const Array<T, N>& lhs,
                                   const Array<T, N>& rhs,
                                   size_t current_index) noexcept -> bool {
  return (current_index == N)
             ? true
             : (lhs.at(current_index) == rhs.at(current_index) &&
                array_equals(lhs, rhs, current_index + 1));
}

template <class T, size_t N>
constexpr inline auto array_less(const Array<T, N>& lhs, const Array<T, N>& rhs,
                                 size_t current_index) noexcept -> bool {
  return (current_index == N)
             ? false
             : (lhs.at(current_index) < rhs.at(current_index) ||
                array_less(lhs, rhs, current_index + 1));
}

}  // namespace Internal

template <class T, size_t N>
constexpr inline auto operator==(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return Internal::array_equals(one, two, 0);
}

template <class T, size_t N>
constexpr inline auto operator!=(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return !(one == two);
}

template <class T, size_t N>
constexpr inline auto operator<(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return Internal::array_less(one, two, 0);
}

template <class T, size_t N>
constexpr inline auto operator>(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return two < one;
}

template <class T, size_t N>
constexpr inline auto operator<=(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return !(one > two);
}

template <class T, size_t N>
constexpr inline auto operator>=(const Array<T, N>& one, const Array<T, N>& two)
    -> bool {
  return !(one < two);
}

template <typename Tp, size_t N>
inline auto swap(Array<Tp, N>& one, Array<Tp, N>& two) noexcept -> void {
  one.swap(two);
}

template <size_t Int, typename Tp, size_t N>
constexpr auto get(Array<Tp, N>& A) noexcept -> Tp& {
  static_assert(Int < N, "Index out of bounds in domino::get");
  return Internal::ArrayTraits<Tp, N>::ref(A.Elems, Int);
}

template <size_t Int, typename Tp, size_t N>
constexpr auto get(Array<Tp, N>&& A) noexcept -> Tp&& {
  static_assert(Int < N, "Index out of bounds in domino::get");
  return std::move(get<Int>(A));
}

template <size_t Int, typename Tp, size_t N>
constexpr auto get(const Array<Tp, N>& A) noexcept -> const Tp& {
  static_assert(Int < N, "Index out of bounds in domino::get");
  return Internal::ArrayTraits<Tp, N>::ref(A.Elems, Int);
}

namespace Internal {

template <class T, size_t N, size_t... Index>
constexpr inline auto tail_(const Array<T, N>& A, std::index_sequence<Index...>)
    -> Array<T, N - 1> {
  static_assert(sizeof...(Index) == N - 1, "invariant");
  return {{get<Index + 1>(A)...}};
}

template <class T, size_t N, size_t... Index>
constexpr inline auto prepend_(T&& head, const Array<T, N>& tail,
                               std::index_sequence<Index...>)
    -> Array<T, N + 1> {
  return {{std::forward<T>(head), get<Index>(tail)...}};
}

template <class T, size_t N, size_t... Index>
constexpr auto to_array_(const T (&arr)[N], std::index_sequence<Index...>)
    -> Array<T, N> {
  return {{arr[Index]...}};
}

}  // namespace Internal

template <class T, size_t N>
constexpr inline auto tail(const Array<T, N>& A) -> Array<T, N - 1> {
  static_assert(N > 0,
                "Can only call tail() on an array with at least one element");
  return Internal::tail_(A, std::make_index_sequence<N - 1>());
}

template <class T, size_t N>
constexpr inline auto prepend(T&& head, const Array<T, N>& tail)
    -> Array<T, N + 1> {
  return Internal::prepend_(std::forward<T>(head), tail,
                            std::make_index_sequence<N>());
}

template <class T, size_t N>
constexpr auto to_array(const T (&arr)[N]) -> Array<T, N> {
  return Internal::to_array_(arr, std::make_index_sequence<N>());
}

}  // namespace RG

#endif  // REGGEN_CONTAINER_ARRAY_H_