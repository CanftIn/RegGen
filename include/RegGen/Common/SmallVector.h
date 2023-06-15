#ifndef REGGEN_COMMON_SMALL_VECTOR_H
#define REGGEN_COMMON_SMALL_VECTOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <ostream>
#include <type_traits>
#include <utility>

namespace RG {

template <class SizeT>
class SmallVectorBase {
 protected:
  void* BeginX;
  SizeT Size = 0, Capacity;

  static constexpr auto SizeTypeMax() -> size_t {
    return std::numeric_limits<SizeT>::max();
  }

  SmallVectorBase(void* first_el, size_t total_capacity)
      : BeginX(first_el), Capacity(total_capacity) {}

  auto mallocForGrow(size_t min_size, size_t t_size, size_t& new_capacity)
      -> void*;

  auto growPod(void* first_el, size_t min_size, size_t t_size) -> void;

 public:
  SmallVectorBase() = delete;

  auto size() const -> size_t { return Size; }

  auto capacity() const -> size_t { return Capacity; }

  auto empty() const -> bool { return !Size; }

  auto set_size(size_t n) -> void {
    assert(n <= capacity());
    Size = n;
  }
};

[[noreturn]] static auto report_size_overflow(size_t min_size, size_t max_size)
    -> void;
static auto report_size_overflow(size_t min_size, size_t max_size) -> void {
  std::string reason = "SmallVector unable to grow. Requested capacity (" +
                       std::to_string(min_size) +
                       ") is larger than maximum value for size type (" +
                       std::to_string(max_size) + ")";
  throw std::length_error(reason);
}

[[noreturn]] static auto report_at_maximum_capacity(size_t max_size) -> void;
static auto report_at_maximum_capacity(size_t max_size) -> void {
  std::string reason =
      "SmallVector capacity unable to grow. Already at maximum size " +
      std::to_string(max_size);
  throw std::length_error(reason);
}

template <class SizeT>
static auto getNewCapacity(size_t min_size, size_t old_capacity) -> size_t {
  constexpr size_t max_size = std::numeric_limits<SizeT>::max();

  if (min_size > max_size) {
    report_size_overflow(min_size, max_size);
  }

  if (old_capacity == max_size) {
    report_at_maximum_capacity(max_size);
  }

  size_t new_capacity = 2 * old_capacity + 1;
  return std::min(std::max(new_capacity, min_size), max_size);
}

template <class SizeT>
auto SmallVectorBase<SizeT>::mallocForGrow(size_t min_size, size_t t_size,
                                           size_t& new_capacity) -> void* {
  new_capacity = getNewCapacity<SizeT>(min_size, this->capacity());
  auto* result = std::malloc(new_capacity * t_size);
  if (result == nullptr) {
    throw std::bad_alloc();
  }
  return result;
}

template <class SizeT>
auto SmallVectorBase<SizeT>::growPod(void* first_el, size_t min_size,
                                     size_t t_size) -> void {
  size_t new_capacity =
      getNewCapacity<SizeT>(min_size, this->capacity());
  void* new_elts = nullptr;
  if (BeginX == first_el) {
    new_elts = std::malloc(new_capacity * t_size);
    if (new_elts == nullptr) {
      throw std::bad_alloc();
    }

    memcpy(new_elts, this->BeginX, size() * t_size);
  } else {
    new_elts = std::realloc(this->BeginX, new_capacity * t_size);
    if (new_elts == nullptr) {
      throw std::bad_alloc();
    }
  }
  this->BeginX = new_elts;
  this->Capacity = new_capacity;
}

template <class T>
using SmallVectorSizeType =
    std::conditional_t<sizeof(T) < 4 && sizeof(void*) >= 8, uint64_t, uint32_t>;

template <class T, typename = void>
struct SmallVectorAlignmentAndSize {
  alignas(SmallVectorBase<SmallVectorSizeType<T>>) char Base[sizeof(
      SmallVectorBase<SmallVectorSizeType<T>>)];
  alignas(T) char FirstEl[sizeof(T)];
};

template <typename T, typename = void>
class SmallVectorTemplateCommon
    : public SmallVectorBase<SmallVectorSizeType<T>> {
  using Base = SmallVectorBase<SmallVectorSizeType<T>>;

  auto getFirstEl() const -> void* {
    return const_cast<void*>(reinterpret_cast<const void*>(
        reinterpret_cast<const char*>(this) +
        offsetof(SmallVectorAlignmentAndSize<T>, FirstEl)));
  }

 protected:
  SmallVectorTemplateCommon(size_t size) : Base(getFirstEl(), size) {}

  auto growPod(size_t min_size, size_t t_size) -> void {
    Base::growPod(getFirstEl(), min_size, t_size);
  }

  auto isSmall() const -> bool { return this->BeginX == getFirstEl(); }

  auto resetToSmall() -> void {
    this->BeginX = getFirstEl();
    this->Size = this->Capacity = 0;
  }

  auto isReferenceToRange(const void* v, const void* first,
                          const void* last) const -> bool {
    std::less<> less_than;
    return !less_than(v, first) && less_than(v, last);
  }

  auto isReferenceToStorage(const void* v) -> bool {
    return isReferenceToRange(v, this->begin(), this->end());
  }

  auto isRangeInStorage(const void* first, const void* last) const -> bool {
    std::less<> less_than;
    return !less_than(first, this->begin()) && !less_than(last, first) &&
           !less_than(this->end(), last);
  }

  auto isSafeToReferenceAfterResize(const void* elt, size_t new_size) -> bool {
    if (!isReferenceToStorage(elt)) {
      return true;
    }

    if (new_size <= this->size()) {
      return elt < this->begin() + new_size;
    }

    return new_size <= this->capacity();
  }

  auto assertSafeToReferenceAfterResize(const void* elt, size_t new_size)
      -> void {
    (void)elt;
    (void)new_size;
    assert(isSafeToReferenceAfterResize(elt, new_size) &&
           "Attempting to reference an element of the vector in an operation "
           "that invalidates it");
  }

  auto assertSafeToAdd(const void* elt, size_t n = 1) -> void {
    this->assertSafeToReferenceAfterResize(elt, this->size() + n);
  }

  auto assertSafeToReferenceAfterClear(const T* from, const T* to) {
    if (from == to) {
      return;
    }
    this->assertSafeToReferenceAfterResize(from, 0);
    this->assertSafeToReferenceAfterResize(to - 1, 0);
  }

  template <class ItTy,
            std::enable_if_t<!std::is_same_v<std::remove_const_t<ItTy>, T*>,
                             bool> = false>
  auto assertSafeToReferenceAfterClear(ItTy /*unused*/, ItTy /*unused*/)
      -> void {}

  auto assertSafeToAddRange(const T* from, const T* to) -> void {
    if (from == to) {
      return;
    }
    this->assertSafeToAdd(from, to - from);
    this->assertSafeToAdd(to - 1, to - from);
  }

  template <class ItTy,
            std::enable_if_t<!std::is_same_v<std::remove_const_t<ItTy>, T*>,
                             bool> = false>
  auto assertSafeToAddRange(ItTy /*unused*/, ItTy /*unused*/) -> void {}

  template <class U>
  static auto reserveForParamAndGetAddressImpl(U* this_obj, const T& elt,
                                               size_t n) -> const T* {
    size_t new_size = this_obj->size() + n;
    if (new_size <= this_obj->capacity()) {
      return &elt;
    }

    bool references_storage = false;
    int64_t index = -1;
    if (!U::takes_param_by_value) {
      if (this_obj->isReferenceToStorage(&elt)) {
        references_storage = true;
        index = &elt - this_obj->begin();
      }
    }
    this_obj->grow(new_size);
    return references_storage ? this_obj->begin() + index : &elt;
  }

 public:
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using value_type = T;
  using iterator = T*;
  using const_iterator = const T*;

  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;

  using Base::capacity;
  using Base::empty;
  using Base::size;

  auto begin() -> iterator { return static_cast<iterator>(this->BeginX); }

  auto begin() const -> const_iterator {
    return static_cast<const_iterator>(this->BeginX);
  }

  auto end() -> iterator { return begin() + size(); }

  auto end() const -> const_iterator { return begin() + size(); }

  auto rbegin() -> reverse_iterator { return reverse_iterator(end()); }

  auto rbegin() const -> const_reverse_iterator {
    return const_reverse_iterator(end());
  }

  auto rend() -> reverse_iterator { return reverse_iterator(begin()); }

  auto rend() const -> const_reverse_iterator {
    return const_reverse_iterator(begin());
  }

  auto size_in_bytes() const -> size_type { return size() * sizeof(T); }

  auto max_size() const -> size_type {
    return std::min(this->SizeTypeMax(), static_cast<size_type>(-1) / sizeof(T));
  }

  auto capacity_in_bytes() const -> size_t { return capacity() * sizeof(T); }

  auto data() -> pointer { return pointer(begin()); }

  auto data() const -> const_pointer { return const_pointer(begin()); }

  auto at(size_type idx) -> reference {
    assert(idx < size());
    return begin()[idx];
  }

  auto at(size_type idx) const -> const_reference {
    assert(idx < size());
    return begin()[idx];
  }

  auto operator[](size_type idx) -> reference {
    assert(idx < size());
    return begin()[idx];
  }

  auto operator[](size_type idx) const -> const_reference {
    assert(idx < size());
    return begin()[idx];
  }

  auto front() -> reference {
    assert(!empty());
    return begin()[0];
  }

  auto front() const -> const_reference {
    assert(!empty());
    return begin()[0];
  }

  auto back() -> reference {
    assert(!empty());
    return end()[-1];
  }

  auto back() const -> const_reference {
    assert(!empty());
    return end()[-1];
  }
};

template <typename T, bool = (std::is_trivially_copy_constructible_v<T>)&&(
                          std::is_trivially_move_constructible_v<
                              T>)&&(std::is_trivially_destructible_v<T>)>
class SmallVectorTemplateBase : public SmallVectorTemplateCommon<T> {
  friend class SmallVectorTemplateCommon<T>;

 protected:
  static constexpr bool takes_param_by_value = false;
  using ValueParamT = const T&;

  SmallVectorTemplateBase(size_t size) : SmallVectorTemplateCommon<T>(size) {}

  static auto destroy_range(T* s, T* e) -> void {
    while (s != e) {
      --e;
      e->~T();
    }
  }

  /// Move the range [i, e) into the uninitialized memory starting with "dest",
  /// constructing elements as needed.
  template <typename It1, typename It2>
  static auto uninitialized_move(It1 i, It1 e, It2 dest) -> void {
    std::uninitialized_copy(std::make_move_iterator(i),
                            std::make_move_iterator(e), dest);
  }

  /// Copy the range [i, e) into the uninitialized memory starting with "dest",
  /// constructing elements as needed.
  template <typename It1, typename It2>
  static auto uninitialized_copy(It1 i, It1 e, It2 dest) -> void {
    std::uninitialized_copy(i, e, dest);
  }

  auto grow(size_t min_size = 0) -> void;

  auto mallocForGrow(size_t min_size, size_t& new_capacity) -> T* {
    return static_cast<T*>(
        SmallVectorBase<SmallVectorSizeType<T>>::mallocForGrow(
            min_size, sizeof(T), new_capacity));
  }

  auto moveElementsForGrow(T* new_elts) -> void;

  auto takeAllocationForGrow(T* new_elts, size_t new_capacity) -> void;

  /// Reserve enough space to add one element, and return the updated element
  /// pointer in case it was a reference to the storage.
  auto reserveForParamAndGetAddress(const T& elt, size_t n = 1) -> const T* {
    return this->reserveForParamAndGetAddressImpl(this, elt, n);
  }

  /// Reserve enough space to add one element, and return the updated element
  /// pointer in case it was a reference to the storage.
  auto reserveForParamAndGetAddress(T& elt, size_t n = 1) -> T* {
    return const_cast<T*>(this->reserveForParamAndGetAddressImpl(this, elt, n));
  }

  static auto forward_value_param(T&& v) -> T&& { return std::move(v); }

  static auto forward_value_param(const T& v) -> const T& { return v; }

  void growAndAssign(size_t num_elts, const T& elt) {
    size_t new_capacity = 0;
    T* new_elts = mallocForGrow(num_elts, new_capacity);
    std::uninitialized_fill_n(new_elts, num_elts, elt);
    this->destroy_range(this->begin(), this->end());
    takeAllocationForGrow(new_elts, new_capacity);
    this->set_size(num_elts);
  }

  template <typename... ArgTypes>
  auto growAndEmplaceBack(ArgTypes&&... args) -> T& {
    size_t new_capacity = 0;
    T* new_elts = mallocForGrow(0, new_capacity);
    ::new (static_cast<void*>(new_elts + this->size()))
        T(std::forward<ArgTypes>(args)...);
    moveElementsForGrow(new_elts);
    takeAllocationForGrow(new_elts, new_capacity);
    this->set_size(this->size() + 1);
    return this->back();
  }

 public:
  auto push_back(const T& elt) -> void {
    const T* elt_ptr = reserveForParamAndGetAddress(elt);
    ::new (static_cast<void*>(this->end())) T(*elt_ptr);
    this->set_size(this->size() + 1);
  }

  auto push_back(T&& elt) -> void {
    T* elt_ptr = reserveForParamAndGetAddress(elt);
    ::new (static_cast<void*>(this->end())) T(::std::move(*elt_ptr));
    this->set_size(this->size() + 1);
  }

  auto pop_back() -> void {
    this->set_size(this->size() - 1);
    this->end()->~T();
  }
};

template <typename T, bool TriviallyCopyable>
auto SmallVectorTemplateBase<T, TriviallyCopyable>::grow(size_t min_size)
    -> void {
  size_t new_capacity = 0;
  T* new_elts = mallocForGrow(min_size, new_capacity);
  moveElementsForGrow(new_elts);
  takeAllocationForGrow(new_elts, new_capacity);
}

template <typename T, bool TriviallyCopyable>
auto SmallVectorTemplateBase<T, TriviallyCopyable>::moveElementsForGrow(
    T* new_elts) -> void {
  this->uninitialized_move(this->begin(), this->end(), new_elts);
  destroy_range(this->begin(), this->end());
}

template <typename T, bool TriviallyCopyable>
auto SmallVectorTemplateBase<T, TriviallyCopyable>::takeAllocationForGrow(
    T* new_elts, size_t new_capacity) -> void {
  if (!this->isSmall()) {
    free(this->begin());
  }

  this->BeginX = new_elts;
  this->Capacity = new_capacity;
}

template <typename T>
class SmallVectorTemplateBase<T, true> : public SmallVectorTemplateCommon<T> {
  friend class SmallVectorTemplateCommon<T>;

 protected:
  static constexpr bool takes_param_by_value = sizeof(T) <= 2 * sizeof(void*);

  using ValueParamT = std::conditional_t<takes_param_by_value, T, const T&>;

  SmallVectorTemplateBase(size_t size)
      : SmallVectorTemplateCommon<T>(size) {}

  // No need to do a destroy loop for POD's.
  static auto destroy_range(T* /*unused*/, T* /*unused*/) -> void {}

  template <typename It1, typename It2>
  static auto uninitialized_move(It1 i, It1 e, It2 dest) -> void {
    uninitialized_copy(i, e, dest);
  }

  template <typename It1, typename It2>
  static auto uninitialized_copy(It1 i, It1 e, It2 dest) -> void {
    std::uninitialized_copy(i, e, dest);
  }

  template <typename T1, typename T2>
  static auto uninitialized_copy(
      T1* i, T1* e, T2* dest,
      std::enable_if_t<std::is_same_v<std::remove_const_t<T1>, T2>>* /*unused*/
      = nullptr) -> void {
    if (i != e) {
      memcpy(reinterpret_cast<void*>(dest), i, (e - i) * sizeof(T));
    }
  }

  auto grow(size_t min_size = 0) -> void { this->growPod(min_size, sizeof(T)); }

  auto reserveForParamAndGetAddress(const T& elt, size_t n = 1) -> const T* {
    return this->reserveForParamAndGetAddressImpl(this, elt, n);
  }

  auto reserveForParamAndGetAddress(T& elt, size_t n = 1) -> T* {
    return const_cast<T*>(this->reserveForParamAndGetAddressImpl(this, elt, n));
  }

  static auto forward_value_param(ValueParamT v) -> ValueParamT { return v; }

  auto growAndAssign(size_t num_elts, T elt) -> void {
    this->set_size(0);
    this->grow(num_elts);
    std::uninitialized_fill_n(this->begin(), num_elts, elt);
    this->set_size(num_elts);
  }

  template <typename... ArgTypes>
  auto growAndEmplaceBack(ArgTypes&&... args) -> T& {
    push_back(T(std::forward<ArgTypes>(args)...));
    return this->back();
  }

 public:
  auto push_back(ValueParamT elt) -> void {
    const T* elt_ptr = reserveForParamAndGetAddress(elt);
    memcpy(reinterpret_cast<void*>(this->end()), elt_ptr, sizeof(T));
    this->set_size(this->size() + 1);
  }

  auto pop_back() -> void { this->set_size(this->size() - 1); }
};

template <typename T>
class SmallVectorImpl : public SmallVectorTemplateBase<T> {
  using SuperClass = SmallVectorTemplateBase<T>;

 public:
  using iterator = typename SuperClass::iterator;
  using const_iterator = typename SuperClass::const_iterator;
  using reference = typename SuperClass::reference;
  using size_type = typename SuperClass::size_type;

 protected:
  using SmallVectorTemplateBase<T>::takes_param_by_value;
  using ValueParamT = typename SuperClass::ValueParamT;

  explicit SmallVectorImpl(unsigned n) : SmallVectorTemplateBase<T>(n) {}

 public:
  SmallVectorImpl(const SmallVectorImpl&) = delete;

  ~SmallVectorImpl() {
    if (!this->isSmall()) {
      free(this->begin());
    }
  }

  auto clear() -> void {
    this->destroy_range(this->begin(), this->end());
    this->Size = 0;
  }

 private:
  template <bool ForOverwrite>
  auto resizeImpl(size_type n) -> void {
    if (n < this->size()) {
      this->pop_back_n(this->size() - n);
    } else if (n > this->size()) {
      this->reserve(n);
      for (auto i = this->end(), e = this->begin() + n; i != e; ++i) {
        if (ForOverwrite) {
          new (&*i) T;
        } else {
          new (&*i) T();
        }
      }
      this->set_size(n);
    }
  }

 public:
  auto resize(size_type n) -> void { resizeImpl<false>(n); }

  auto resize_for_overwrite(size_type n) -> void { resizeImpl<true>(n); }

  auto truncate(size_type n) -> void {
    assert(this->size() >= n && "Cannot increase size with truncate");
    this->destroy_range(this->begin() + n, this->end());
    this->set_size(n);
  }

  auto resize(size_type n, ValueParamT nv) -> void {
    if (n == this->size()) {
      return;
    }

    if (n < this->size()) {
      this->pop_back_n(this->size() - n);
      return;
    }

    // N > this->size(). Defer to append.
    this->append(n - this->size(), nv);
  }

  auto reserve(size_type n) -> void {
    if (this->capacity() < n) {
      this->grow(n);
    }
  }

  auto pop_back_n(size_type num_items) -> void {
    assert(this->size() >= num_items);
    this->destroy_range(this->end() - num_items, this->end());
    this->set_size(this->size() - num_items);
  }

  [[nodiscard]] auto pop_back_val() -> T {
    T result = ::std::move(this->back());
    this->pop_back();
    return result;
  }

  auto swap(SmallVectorImpl& rhs) -> void;

  template <typename InIter,
            typename = std::enable_if_t<std::is_convertible_v<
                typename std::iterator_traits<InIter>::iterator_category,
                std::input_iterator_tag>>>
  void append(InIter in_start, InIter in_end) {
    this->assertSafeToAddRange(in_start, in_end);
    size_type num_inputs = std::distance(in_start, in_end);
    this->reserve(this->size() + num_inputs);
    this->uninitialized_copy(in_start, in_end, this->end());
    this->set_size(this->size() + num_inputs);
  }

  auto append(size_type num_inputs, ValueParamT elt) -> void {
    const T* elt_ptr = this->reserveForParamAndGetAddress(elt, num_inputs);
    std::uninitialized_fill_n(this->end(), num_inputs, *elt_ptr);
    this->set_size(this->size() + num_inputs);
  }

  auto append(std::initializer_list<T> il) -> void {
    append(il.begin(), il.end());
  }

  auto append(const SmallVectorImpl& rhs) -> void {
    append(rhs.begin(), rhs.end());
  }

  auto assign(size_type num_elts, ValueParamT elt) -> void {
    if (num_elts > this->capacity()) {
      this->growAndAssign(num_elts, elt);
      return;
    }
    std::fill_n(this->begin(), std::min(num_elts, this->size()), elt);
    if (num_elts > this->size()) {
      std::uninitialized_fill_n(this->end(), num_elts - this->size(), elt);
    } else if (num_elts < this->size()) {
      this->destroy_range(this->begin() + num_elts, this->end());
    }
    this->set_size(num_elts);
  }

  template <typename InIter,
            typename = std::enable_if_t<std::is_convertible_v<
                typename std::iterator_traits<InIter>::iterator_category,
                std::input_iterator_tag>>>
  auto assign(InIter in_start, InIter in_end) -> void {
    this->assertSafeToReferenceAfterClear(in_start, in_end);
    clear();
    append(in_start, in_end);
  }

  auto assign(std::initializer_list<T> il) -> void {
    clear();
    append(il);
  }

  auto assign(const SmallVectorImpl& rhs) -> void {
    assign(rhs.begin(), rhs.end());
  }

  auto erase(const_iterator ci) -> const_iterator {
    auto i = const_cast<iterator>(ci);

    assert(this->isReferenceToStorage(ci) &&
           "Iterator to erase is out of bounds.");

    iterator n = i;
    std::move(i + 1, this->end(), i);
    this->pop_back();
    return (n);
  }

  auto erase(const_iterator cs, const_iterator ce) -> const_iterator {
    auto s = const_cast<iterator>(cs);
    auto e = const_cast<iterator>(ce);

    assert(this->isRangeInStorage(s, e) && "Range to erase is out of bounds.");

    iterator n = s;
    iterator i = std::move(e, this->end(), s);
    this->destroy_range(i, this->end());
    this->set_size(i - this->begin());
    return (n);
  }

 private:
  template <class ArgType>
  auto insert_one_impl(iterator i, ArgType&& elt) -> iterator {
    static_assert(
        std::is_same<std::remove_const_t<std::remove_reference_t<ArgType>>,
                     T>::value,
        "ArgType must be derived from T!");
    if (i == this->end()) {
      this->push_back(::std::forward<ArgType>(elt));
      return this->end() - 1;
    }

    assert(this->isReferenceToStorage(i) &&
           "Insertion iterator is out of bounds.");

    size_t index = i - this->begin();
    std::remove_reference_t<ArgType>* elt_ptr =
        this->reserveForParamAndGetAddress(elt);
    i = this->begin() + index;

    ::new (static_cast<void*>(this->end())) T(::std::move(this->back()));

    std::move_backward(i, this->end() - 1, this->end());
    this->set_size(this->size() + 1);

    static_assert(!takes_param_by_value || std::is_same<ArgType, T>::value,
                  "ArgType must be 'T' when taking by value!");
    if (!takes_param_by_value &&
        this->isReferenceToRange(elt_ptr, i, this->end())) {
      ++elt_ptr;
    }

    *i = ::std::forward<ArgType>(*elt_ptr);
    return i;
  }

 public:
  auto insert(iterator i, T&& elt) -> iterator {
    return insert_one_impl(i, this->forward_value_param(std::move(elt)));
  }

  auto insert(iterator i, const T& elt) -> iterator {
    return insert_one_impl(i, this->forward_value_param(elt));
  }

  auto insert(iterator i, size_type num_to_insert, ValueParamT elt) -> iterator {
    size_t insert_elt = i - this->begin();

    if (i == this->end()) {
      append(num_to_insert, elt);
      return this->begin() + insert_elt;
    }

    assert(this->isReferenceToStorage(i) &&
           "Insertion iterator is out of bounds.");

    const T* elt_ptr = this->reserveForParamAndGetAddress(elt, num_to_insert);
    i = this->begin() + insert_elt;

    if (size_t(this->end() - i) >= num_to_insert) {
      T* old_end = this->end();
      append(std::move_iterator<iterator>(this->end() - num_to_insert),
             std::move_iterator<iterator>(this->end()));
      std::move_backward(i, old_end - num_to_insert, old_end);

      if (!takes_param_by_value && i <= elt_ptr && elt_ptr < this->end()) {
        elt_ptr += num_to_insert;
      }

      std::fill_n(i, num_to_insert, *elt_ptr);
      return i;
    }

    T* old_end = this->end();
    this->set_size(this->size() + num_to_insert);
    size_t num_overwritten = old_end - i;
    this->uninitialized_move(i, old_end, this->end() - num_overwritten);

    if (!takes_param_by_value && i <= elt_ptr && elt_ptr < this->end()) {
      elt_ptr += num_to_insert;
    }

    std::fill_n(i, num_overwritten, *elt_ptr);
    std::uninitialized_fill_n(old_end, num_to_insert - num_overwritten,
                              *elt_ptr);
    return i;
  }

  template <typename ItTy,
            typename = std::enable_if_t<std::is_convertible_v<
                typename std::iterator_traits<ItTy>::iterator_category,
                std::input_iterator_tag>>>
  auto insert(iterator i, ItTy from, ItTy to) -> iterator {
    size_t insert_elt = i - this->begin();

    if (i == this->end()) {
      append(from, to);
      return this->begin() + insert_elt;
    }

    assert(this->isReferenceToStorage(i) &&
           "Insertion iterator is out of bounds.");

    this->assertSafeToAddRange(from, to);
    size_t num_to_insert = std::distance(from, to);
    reserve(this->size() + num_to_insert);
    i = this->begin() + insert_elt;

    if (size_t(this->end() - i) >= num_to_insert) {
      T* old_end = this->end();
      append(std::move_iterator<iterator>(this->end() - num_to_insert),
             std::move_iterator<iterator>(this->end()));
      std::move_backward(i, old_end - num_to_insert, old_end);
      std::copy(from, to, i);
      return i;
    }

    T* old_end = this->end();
    this->set_size(this->size() + num_to_insert);
    size_t num_overwritten = old_end - i;
    this->uninitialized_move(i, old_end, this->end() - num_overwritten);

    for (T* j = i; num_overwritten > 0; --num_overwritten) {
      *j = *from;
      ++j;
      ++from;
    }

    this->uninitialized_copy(from, to, old_end);
    return i;
  }

  auto insert(iterator i, std::initializer_list<T> il) -> iterator {
    insert(i, il.begin(), il.end());
  }

  template <typename... ArgTypes>
  auto emplace_back(ArgTypes&&... args) -> reference {
    if (this->size() >= this->capacity()) {
      return this->growAndEmplaceBack(std::forward<ArgTypes>(args)...);
    }

    ::new (static_cast<void*>(this->end())) T(std::forward<ArgTypes>(args)...);
    this->set_size(this->size() + 1);
    return this->back();
  }

  auto operator=(const SmallVectorImpl& rhs) -> SmallVectorImpl&;

  auto operator=(SmallVectorImpl&& rhs) noexcept -> SmallVectorImpl&;

  auto operator==(const SmallVectorImpl& rhs) const -> bool {
    if (this->size() != rhs.size()) {
      return false;
    }
    return std::equal(this->begin(), this->end(), rhs.begin());
  }

  auto operator!=(const SmallVectorImpl& rhs) const -> bool {
    return !(*this == rhs);
  }

  auto operator<(const SmallVectorImpl& rhs) const -> bool {
    return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(),
                                        rhs.end());
  }
};

template <typename T>
auto SmallVectorImpl<T>::swap(SmallVectorImpl<T>& rhs) -> void {
  if (this == &rhs) {
    return;
  }

  if (!this->isSmall() && !rhs.isSmall()) {
    std::swap(this->BeginX, rhs.BeginX);
    std::swap(this->Size, rhs.Size);
    std::swap(this->Capacity, rhs.Capacity);
    return;
  }
  this->reserve(rhs.size());
  rhs.reserve(this->size());

  size_t num_shared = this->size();
  if (num_shared > rhs.size()) {
    num_shared = rhs.size();
  }

  for (size_type i = 0; i != num_shared; ++i) {
    std::swap((*this)[i], rhs[i]);
  }

  if (this->size() > rhs.size()) {
    size_t elt_diff = this->size() - rhs.size();
    this->uninitialized_copy(this->begin() + num_shared, this->end(),
                             rhs.end());
    rhs.set_size(rhs.size() + elt_diff);
    this->destroy_range(this->begin() + num_shared, this->end());
    this->set_size(num_shared);
  } else if (rhs.size() > this->size()) {
    size_t elt_diff = rhs.size() - this->size();
    this->uninitialized_copy(rhs.begin() + num_shared, rhs.end(), this->end());
    this->set_size(this->size() + elt_diff);
    this->destroy_range(rhs.begin() + num_shared, rhs.end());
    rhs.set_size(num_shared);
  }
}

template <typename T>
auto SmallVectorImpl<T>::operator=(const SmallVectorImpl<T>& rhs)
    -> SmallVectorImpl<T>& {
  if (this == &rhs) {
    return *this;
  }

  size_t rhs_size = rhs.size();
  size_t cur_size = this->size();
  if (rhs_size <= cur_size) {
    iterator new_end;
    if (rhs_size) {
      new_end = std::copy(rhs.begin(), rhs.begin() + rhs_size, this->begin());
    } else {
      new_end = this->begin();
    }

    this->destroy_range(new_end, this->end());
    this->set_size(rhs_size);
    return *this;
  }

  if (this->capacity() < rhs_size) {
    this->clear();
    cur_size = 0;
    this->grow(rhs_size);
  } else if (cur_size) {
    std::copy(rhs.begin(), rhs.begin() + cur_size, this->begin());
  }

  this->uninitialized_copy(rhs.begin() + cur_size, rhs.end(),
                           this->begin() + cur_size);
  this->set_size(rhs_size);
  return *this;
}

template <typename T>
auto SmallVectorImpl<T>::operator=(SmallVectorImpl<T>&& rhs) noexcept
    -> SmallVectorImpl<T>& {
  if (this == &rhs) {
    return *this;
  }

  if (!rhs.isSmall()) {
    this->destroy_range(this->begin(), this->end());
    if (!this->isSmall()) {
      free(this->begin());
    }
    this->BeginX = rhs.BeginX;
    this->Size = rhs.Size;
    this->Capacity = rhs.Capacity;
    rhs.resetToSmall();
    return *this;
  }

  size_t rhs_size = rhs.size();
  size_t cur_size = this->size();

  if (cur_size >= rhs_size) {
    iterator new_end = this->begin();
    if (rhs_size) {
      new_end = std::move(rhs.begin(), rhs.end(), new_end);
    }

    this->destroy_range(new_end, this->end());
    this->set_size(rhs_size);
    rhs.clear();
    return *this;
  }

  if (this->capacity() < rhs_size) {
    this->clear();
    cur_size = 0;
    this->grow(rhs_size);
  } else if (cur_size) {
    std::move(rhs.begin(), rhs.begin() + cur_size, this->begin());
  }

  this->uninitialized_move(rhs.begin() + cur_size, rhs.end(),
                           this->begin() + cur_size);
  this->set_size(rhs_size);
  rhs.clear();
  return *this;
}

template <typename T, unsigned N>
struct SmallVectorStorage {
  alignas(T) char InlineElts[N * sizeof(T)];
};

template <typename T>
struct alignas(T) SmallVectorStorage<T, 0> {};

template <typename T, unsigned N>
class SmallVector;

template <typename T>
struct CalculateSmallVectorDefaultInlinedElements {
  static constexpr size_t preferred_small_vector_sizeof = 64;
  static_assert(
      sizeof(T) <= 256,
      "You are trying to use a default number of inlined elements for "
      "`SmallVector<T>` but `sizeof(T)` is really big! Please use an "
      "explicit number of inlined elements with `SmallVector<T, N>` to make "
      "sure you really want that much inline storage.");
  static constexpr size_t preferred_inline_bytes =
      preferred_small_vector_sizeof - sizeof(SmallVector<T, 0>);
  static constexpr size_t num_elements_that_fit =
      preferred_inline_bytes / sizeof(T);
  static constexpr size_t value =
      num_elements_that_fit == 0 ? 1 : num_elements_that_fit;
};

template <typename T,
          unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
class SmallVector : public SmallVectorImpl<T>, SmallVectorStorage<T, N> {
 public:
  SmallVector() : SmallVectorImpl<T>(N) {}

  ~SmallVector() { this->destroy_range(this->begin(), this->end()); }

  explicit SmallVector(size_t size, const T& value = T())
      : SmallVectorImpl<T>(N) {
    this->assign(size, value);
  }

  template <typename ItTy,
            typename = std::enable_if_t<std::is_convertible_v<
                typename std::iterator_traits<ItTy>::iterator_category,
                std::input_iterator_tag>>>
  SmallVector(ItTy s, ItTy e) : SmallVectorImpl<T>(N) {
    this->append(s, e);
  }

  template <
      typename Container,
      std::enable_if_t<
          std::is_convertible_v<typename std::iterator_traits<
                                    decltype(std::declval<Container>()
                                                 .begin())>::iterator_category,
                                std::input_iterator_tag> &&
              std::is_convertible_v<
                  typename std::iterator_traits<
                      decltype(std::declval<Container>()
                                   .end())>::iterator_category,
                  std::input_iterator_tag>,
          int> = 0>
  explicit SmallVector(Container&& c) : SmallVectorImpl<T>(N) {
    this->append(c.begin(), c.end());
  }

  SmallVector(std::initializer_list<T> il) : SmallVectorImpl<T>(N) {
    this->assign(il);
  }

  SmallVector(const SmallVector& rhs) : SmallVectorImpl<T>(N) {
    if (!rhs.empty()) {
      SmallVectorImpl<T>::operator=(rhs);
    }
  }

  auto operator=(const SmallVector& rhs) -> SmallVector& {
    SmallVectorImpl<T>::operator=(rhs);
    return *this;
  }

  SmallVector(SmallVector&& rhs) noexcept : SmallVectorImpl<T>(N) {
    if (!rhs.empty()) {
      SmallVectorImpl<T>::operator=(::std::move(rhs));
    }
  }

  template <
      typename Container,
      std::enable_if_t<
          std::is_convertible_v<typename std::iterator_traits<
                                    decltype(std::declval<Container>()
                                                 .begin())>::iterator_category,
                                std::input_iterator_tag> &&
              std::is_convertible_v<
                  typename std::iterator_traits<
                      decltype(std::declval<Container>()
                                   .end())>::iterator_category,
                  std::input_iterator_tag>,
          int> = 0>
  auto operator=(const Container& rhs) -> SmallVector& {
    this->assign(rhs.begin(), rhs.end());
    return *this;
  }

  SmallVector(SmallVectorImpl<T>&& rhs) : SmallVectorImpl<T>(N) {
    if (!rhs.empty()) {
      SmallVectorImpl<T>::operator=(::std::move(rhs));
    }
  }

  auto operator=(SmallVector&& rhs) noexcept -> SmallVector& {
    SmallVectorImpl<T>::operator=(::std::move(rhs));
    return *this;
  }

  auto operator=(SmallVectorImpl<T>&& rhs) -> SmallVector& {
    SmallVectorImpl<T>::operator=(::std::move(rhs));
    return *this;
  }

  template <
      typename Container,
      std::enable_if_t<
          std::is_convertible_v<typename std::iterator_traits<
                                    decltype(std::declval<Container>()
                                                 .begin())>::iterator_category,
                                std::input_iterator_tag> &&
              std::is_convertible_v<
                  typename std::iterator_traits<
                      decltype(std::declval<Container>()
                                   .end())>::iterator_category,
                  std::input_iterator_tag>,
          int> = 0>
  auto operator=(Container&& c) -> SmallVector& {
    this->assign(c.begin(), c.end());
    return *this;
  }

  auto operator=(std::initializer_list<T> il) -> SmallVector& {
    this->assign(il);
    return *this;
  }
};

template <typename T, unsigned N>
inline auto capacity_in_bytes(const SmallVector<T, N>& x) -> size_t {
  return x.capacity_in_bytes();
}

template <typename T, unsigned N>
auto operator<<(std::ostream& out, const SmallVector<T, N>& list)
    -> std::ostream& {
  int i = 0;
  out << "[";
  for (auto e : list) {
    if (i++ > 0) {
      out << ", ";
    }
    out << e;
  }
  out << "]";
  return out;
}

template <typename RangeType>
using ValueTypeFromRangeType = std::remove_const_t<
    std::remove_reference_t<decltype(*std::begin(std::declval<RangeType&>()))>>;

template <unsigned Size, typename R>
auto to_vector(R&& range) -> SmallVector<ValueTypeFromRangeType<R>, Size> {
  return {std::begin(range), std::end(range)};
}

template <typename R>
auto to_vector(R&& range)
    -> SmallVector<ValueTypeFromRangeType<R>,
                   CalculateSmallVectorDefaultInlinedElements<
                       ValueTypeFromRangeType<R>>::value> {
  return {std::begin(range), std::end(range)};
}

}  // namespace RG

namespace std {

/// Implement std::swap in terms of SmallVector swap.
template <typename T>
inline void swap(RG::SmallVectorImpl<T>& lhs, RG::SmallVectorImpl<T>& rhs) {
  lhs.swap(rhs);
}

/// Implement std::swap in terms of SmallVector swap.
template <typename T, unsigned N>
inline void swap(RG::SmallVector<T, N>& lhs, RG::SmallVector<T, N>& rhs) {
  lhs.swap(rhs);
}

}  // namespace std

#endif  // REGGEN_COMMON_SMALL_VECTOR_H