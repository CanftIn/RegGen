#ifndef REGGEN_COMMON_FLAT_SET_H
#define REGGEN_COMMON_FLAT_SET_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

namespace RG {

namespace Internal {

template <typename Key, typename Bucket, class Hash, class Eq>
class FlatRep {
 public:
  // kWidth is the number of entries stored in a bucket.
  static constexpr uint32_t kBase = 3;
  static constexpr uint32_t kWidth = (1 << kBase);

  FlatRep(size_t N, const Hash& hf, const Eq& eq) : hash_(hf), equal_(eq) {
    Init(N);
  }
  FlatRep(const FlatRep& src) : hash_(src.hash_), equal_(src.equal_) {
    Init(src.size());
    CopyEntries(src.array_, src.end_, CopyEntry());
  }

  FlatRep(FlatRep&& src) : hash_(src.hash_), equal_(src.equal_) {
    Init(1);
    swap(src);
  }

  ~FlatRep() {
    clear_no_resize();
    delete[] array_;
  }

  auto size() const -> size_t { return not_empty_ - deleted_; }
  auto bucket_count() const -> size_t { return mask_ + 1; }
  auto start() const -> Bucket* { return array_; }
  auto limit() const -> Bucket* { return end_; }
  auto hash_function() const -> const Hash& { return hash_; }
  auto key_eq() const -> const Eq& { return equal_; }

  auto CopyFrom(const FlatRep& src) -> void {
    if (this != &src) {
      clear_no_resize();
      delete[] array_;
      Init(src.size());
      CopyEntries(src.array_, src.end_, CopyEntry());
    }
  }

  auto MoveFrom(FlatRep&& src) -> void {
    if (this != &src) {
      swap(src);
    }
  }

  auto clear_no_resize() -> void {
    for (Bucket* b = array_; b != end_; b++) {
      for (uint32_t i = 0; i < kWidth; i++) {
        if (b->marker[i] >= 2) {
          b->Destroy(i);
          b->marker[i] = kEmpty;
        }
      }
    }
    not_empty_ = 0;
    deleted_ = 0;
  }

  auto clear() -> void {
    clear_no_resize();
    grow_ = 0;
    MaybeResize();
  }

  auto swap(FlatRep& x) -> void {
    using std::swap;
    swap(array_, x.array_);
    swap(end_, x.end_);
    swap(lglen_, x.lglen_);
    swap(mask_, x.mask_);
    swap(not_empty_, x.not_empty_);
    swap(deleted_, x.deleted_);
    swap(grow_, x.grow_);
    swap(shrink_, x.shrink_);
  }

  struct SearchResult {
    bool found;
    Bucket* b;
    uint32_t index;
  };

  // Find bucket/index for key k.
  auto Find(const Key& k) const -> SearchResult {
    size_t h = hash_(k);
    const uint32_t marker = Marker(h & 0xff);
    size_t index = (h >> 8) & mask_;
    uint32_t num_probes = 1;
    while (true) {
      uint32_t bi = index & (kWidth - 1);
      Bucket* b = &array_[index >> kBase];
      const uint32_t x = b->marker[bi];
      if (x == marker && equal_(b->key(bi), k)) {
        return {true, b, bi};
      } else if (x == kEmpty) {
        return {false, nullptr, 0};
      }
      index = NextIndex(index, num_probes);
      num_probes++;
    }
  }

  // Find bucket/index for key k, creating a new one if necessary.
  template <typename KeyType>
  auto FindOrInsert(KeyType&& k) -> SearchResult {
    size_t h = hash_(k);
    const uint32_t marker = Marker(h & 0xff);
    size_t index = (h >> 8) & mask_;  // Holds bucket num and index-in-bucket
    uint32_t num_probes = 1;          // Needed for quadratic probing
    Bucket* del = nullptr;            // First encountered deletion for kInsert
    uint32_t di = 0;
    while (true) {
      uint32_t bi = index & (kWidth - 1);
      Bucket* b = &array_[index >> kBase];
      const uint32_t x = b->marker[bi];
      if (x == marker && equal_(b->key(bi), k)) {
        return {true, b, bi};
      } else if (!del && x == kDeleted) {
        // Remember deleted index to use for insertion.
        del = b;
        di = bi;
      } else if (x == kEmpty) {
        if (del) {
          // Store in the first deleted slot we encountered
          b = del;
          bi = di;
          deleted_--;  // not_empty_ does not change
        } else {
          not_empty_++;
        }
        b->marker[bi] = marker;
        new (&b->key(bi)) Key(std::forward<KeyType>(k));
        return {false, b, bi};
      }
      index = NextIndex(index, num_probes);
      num_probes++;
    }
  }

  auto Erase(Bucket* b, uint32_t i) -> void {
    b->Destroy(i);
    b->marker[i] = kDeleted;
    deleted_++;
    grow_ = 0;  // Consider shrinking on next insert
  }

  auto Prefetch(const Key& k) const -> void {
    size_t h = hash_(k);
    size_t index = (h >> 8) & mask_;  // Holds bucket num and index-in-bucket
    uint32_t bi = index & (kWidth - 1);
    Bucket* b = &array_[index >> kBase];
  }

  inline auto MaybeResize() -> void {
    if (not_empty_ < grow_) {
      return;  // Nothing to do
    }
    if (grow_ == 0) {
      // Special value set by erase to cause shrink on next insert.
      if (size() >= shrink_) {
        // Not small enough to shrink.
        grow_ = static_cast<size_t>(bucket_count() * 0.8);
        if (not_empty_ < grow_) {
          return;
        }
      }
    }
    Resize(size() + 1);
  }

  auto Resize(size_t n) -> void {
    Bucket* old = array_;
    Bucket* old_end = end_;
    Init(n);
    CopyEntries(old, old_end, MoveEntry());
    delete[] old;
  }

 private:
  enum { kEmpty = 0, kDeleted = 1 };  // Special markers for an entry.

  Hash hash_;         // User-supplied hasher
  Eq equal_;          // User-supplied comparator
  uint8_t lglen_;     // lg(#buckets)
  Bucket* array_;     // array of length (1 << lglen_)
  Bucket* end_;       // Points just past last bucket in array_
  size_t mask_;       // (# of entries in table) - 1
  size_t not_empty_;  // Count of entries with marker != kEmpty
  size_t deleted_;    // Count of entries with marker == kDeleted
  size_t grow_;       // Grow array when not_empty_ >= grow_
  size_t shrink_;     // Shrink array when size() < shrink_

  // Avoid kEmpty and kDeleted markers when computing hash values to
  // store in Bucket::marker[].
  static auto Marker(uint32_t hb) -> uint32_t { return hb + (hb < 2 ? 2 : 0); }

  auto Init(size_t N) -> void {
    // Make enough room for N elements.
    size_t lg = 0;  // Smallest table is just one bucket.
    while (N >= 0.8 * ((1 << lg) * kWidth)) {
      lg++;
    }
    const size_t n = (1 << lg);
    auto* array = new Bucket[n];
    for (size_t i = 0; i < n; i++) {
      Bucket* b = &array[i];
      memset(b->marker, kEmpty, kWidth);
    }
    const size_t capacity = (1 << lg) * kWidth;
    lglen_ = lg;
    mask_ = capacity - 1;
    array_ = array;
    end_ = array + n;
    not_empty_ = 0;
    deleted_ = 0;
    grow_ = static_cast<size_t>(capacity * 0.8);
    if (lg == 0) {
      // Already down to one bucket; no more shrinking.
      shrink_ = 0;
    } else {
      shrink_ = static_cast<size_t>(grow_ * 0.4);  // Must be less than 0.5
    }
  }

  // Used by FreshInsert when we should copy from source.
  struct CopyEntry {
    inline void operator()(Bucket* dst, uint32_t dsti, Bucket* src,
                           uint32_t srci) {
      dst->CopyFrom(dsti, src, srci);
    }
  };

  // Used by FreshInsert when we should move from source.
  struct MoveEntry {
    inline void operator()(Bucket* dst, uint32_t dsti, Bucket* src,
                           uint32_t srci) {
      dst->MoveFrom(dsti, src, srci);
      src->Destroy(srci);
      src->marker[srci] = kDeleted;
    }
  };

  template <typename Copier>
  auto CopyEntries(Bucket* start, Bucket* end, Copier copier) -> void {
    for (Bucket* b = start; b != end; b++) {
      for (uint32_t i = 0; i < kWidth; i++) {
        if (b->marker[i] >= 2) {
          FreshInsert(b, i, copier);
        }
      }
    }
  }

  template <typename Copier>
  auto FreshInsert(Bucket* src, uint32_t src_index, Copier copier) -> void {
    size_t h = hash_(src->key(src_index));
    const uint32_t marker = Marker(h & 0xff);
    size_t index = (h >> 8) & mask_;  // Holds bucket num and index-in-bucket
    uint32_t num_probes = 1;          // Needed for quadratic probing
    while (true) {
      uint32_t bi = index & (kWidth - 1);
      Bucket* b = &array_[index >> kBase];
      const uint32_t x = b->marker[bi];
      if (x == 0) {
        b->marker[bi] = marker;
        not_empty_++;
        copier(b, bi, src, src_index);
        return;
      }
      index = NextIndex(index, num_probes);
      num_probes++;
    }
  }

  inline auto NextIndex(size_t i, uint32_t num_probes) const -> size_t {
    // Quadratic probing.
    return (i + num_probes) & mask_;
  }
};

}  // namespace Internal

template <typename Key, class Hash = std::hash<Key>,
          class Eq = std::equal_to<Key>>
class FlatSet {
 private:
  // Forward declare some internal types needed in public section.
  struct Bucket;

 public:
  using key_type = Key;
  using value_type = Key;
  using hasher = Hash;
  using key_equal = Eq;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;

  FlatSet() : FlatSet(1) {}

  explicit FlatSet(size_t N, const Hash& hf = Hash(), const Eq& eq = Eq())
      : rep_(N, hf, eq) {}

  FlatSet(const FlatSet& src) : rep_(src.rep_) {}

  // Move constructor leaves src in a valid but unspecified state (same as
  // std::unordered_set).
  FlatSet(FlatSet&& src) : rep_(std::move(src.rep_)) {}

  template <typename InputIter>
  FlatSet(InputIter first, InputIter last, size_t N = 1,
          const Hash& hf = Hash(), const Eq& eq = Eq())
      : FlatSet(N, hf, eq) {
    insert(first, last);
  }

  FlatSet(std::initializer_list<value_type> init, size_t N = 1,
          const Hash& hf = Hash(), const Eq& eq = Eq())
      : FlatSet(init.begin(), init.end(), N, hf, eq) {}

  auto operator=(const FlatSet& src) -> FlatSet& {
    rep_.CopyFrom(src.rep_);
    return *this;
  }

  auto operator=(FlatSet&& src) -> FlatSet& {
    rep_.MoveFrom(std::move(src.rep_));
    return *this;
  }

  ~FlatSet() {}

  auto swap(FlatSet& x) -> void { rep_.swap(x.rep_); }
  auto clear_no_resize() -> void { rep_.clear_no_resize(); }
  auto clear() -> void { rep_.clear(); }
  auto reserve(size_t N) -> void { rep_.Resize(std::max(N, size())); }
  auto rehash(size_t N) -> void { rep_.Resize(std::max(N, size())); }
  auto resize(size_t N) -> void { rep_.Resize(std::max(N, size())); }
  auto size() const -> size_t { return rep_.size(); }
  auto empty() const -> bool { return size() == 0; }
  auto bucket_count() const -> size_t { return rep_.bucket_count(); }
  auto hash_function() const -> hasher { return rep_.hash_function(); }
  auto key_eq() const -> key_equal { return rep_.key_eq(); }

  class const_iterator {
   public:
    using difference_type = typename FlatSet::difference_type;
    using value_type = typename FlatSet::value_type;
    using pointer = typename FlatSet::const_pointer;
    using reference = typename FlatSet::const_reference;
    using iterator_category = ::std::forward_iterator_tag;

    const_iterator() : b_(nullptr), end_(nullptr), i_(0) {}

    // Make iterator pointing at first element at or after b.
    const_iterator(Bucket* b, Bucket* end) : b_(b), end_(end), i_(0) {
      SkipUnused();
    }

    // Make iterator pointing exactly at ith element in b, which must exist.
    const_iterator(Bucket* b, Bucket* end, uint32_t i)
        : b_(b), end_(end), i_(i) {}

    auto operator*() const -> reference { return key(); }
    auto operator->() const -> pointer { return &key(); }
    auto operator==(const const_iterator& x) const -> bool {
      return b_ == x.b_ && i_ == x.i_;
    }
    auto operator!=(const const_iterator& x) const -> bool {
      return !(*this == x);
    }
    auto operator++() -> const_iterator& {
      assert(b_ != end_);
      i_++;
      SkipUnused();
      return *this;
    }
    auto operator++(int /*indicates postfix*/) -> const_iterator {
      const_iterator tmp(*this);
      ++*this;
      return tmp;
    }

   private:
    friend class FlatSet;
    Bucket* b_;
    Bucket* end_;
    uint32_t i_;

    auto key() const -> reference { return b_->key(i_); }
    auto SkipUnused() -> void {
      while (b_ < end_) {
        if (i_ >= Rep::kWidth) {
          i_ = 0;
          b_++;
        } else if (b_->marker[i_] < 2) {
          i_++;
        } else {
          break;
        }
      }
    }
  };

  using iterator = const_iterator;

  auto begin() -> iterator { return iterator(rep_.start(), rep_.limit()); }
  auto end() -> iterator { return iterator(rep_.limit(), rep_.limit()); }
  auto begin() const -> const_iterator {
    return const_iterator(rep_.start(), rep_.limit());
  }
  auto end() const -> const_iterator {
    return const_iterator(rep_.limit(), rep_.limit());
  }

  auto count(const Key& k) const -> size_t {
    return rep_.Find(k).found ? 1 : 0;
  }
  auto find(const Key& k) -> iterator {
    auto r = rep_.Find(k);
    return r.found ? iterator(r.b, rep_.limit(), r.index) : end();
  }
  auto find(const Key& k) const -> const_iterator {
    auto r = rep_.Find(k);
    return r.found ? const_iterator(r.b, rep_.limit(), r.index) : end();
  }

  auto insert(const Key& k) -> std::pair<iterator, bool> { return Insert(k); }
  auto insert(Key&& k) -> std::pair<iterator, bool> {
    return Insert(std::move(k));
  }
  template <typename InputIter>
  auto insert(InputIter first, InputIter last) -> void {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  template <typename... Args>
  auto emplace(Args&&... args) -> std::pair<iterator, bool> {
    rep_.MaybeResize();
    auto r = rep_.FindOrInsert(std::forward<Args>(args)...);
    const bool inserted = !r.found;
    return {iterator(r.b, rep_.limit(), r.index), inserted};
  }

  auto erase(const Key& k) -> size_t {
    auto r = rep_.Find(k);
    if (!r.found)
      return 0;
    rep_.Erase(r.b, r.index);
    return 1;
  }
  auto erase(iterator pos) -> iterator {
    rep_.Erase(pos.b_, pos.i_);
    ++pos;
    return pos;
  }
  auto erase(iterator pos, iterator last) -> iterator {
    for (; pos != last; ++pos) {
      rep_.Erase(pos.b_, pos.i_);
    }
    return pos;
  }

  auto equal_range(const Key& k) -> std::pair<iterator, iterator> {
    auto pos = find(k);
    if (pos == end()) {
      return std::make_pair(pos, pos);
    } else {
      auto next = pos;
      ++next;
      return std::make_pair(pos, next);
    }
  }
  auto equal_range(const Key& k) const
      -> std::pair<const_iterator, const_iterator> {
    auto pos = find(k);
    if (pos == end()) {
      return std::make_pair(pos, pos);
    } else {
      auto next = pos;
      ++next;
      return std::make_pair(pos, next);
    }
  }

  auto operator==(const FlatSet& x) const -> bool {
    if (size() != x.size()) {
      return false;
    }
    for (const auto& elem : x) {
      auto i = find(elem);
      if (i == end()) {
        return false;
      }
    }
    return true;
  }
  auto operator!=(const FlatSet& x) const -> bool { return !(*this == x); }

  // If key exists in the table, prefetch it.  This is a hint, and may
  // have no effect.
  auto prefetch_value(const Key& key) const -> void { rep_.Prefetch(key); }

 private:
  using Rep = Internal::FlatRep<Key, Bucket, Hash, Eq>;

  // Bucket stores kWidth <marker, key, value> triples.
  // The data is organized as three parallel arrays to reduce padding.
  struct Bucket {
    uint8_t marker[Rep::kWidth];

    // Wrap keys in union to control construction and destruction.
    union Storage {
      Key key[Rep::kWidth];
      Storage() {}
      ~Storage() {}
    } storage;

    auto key(uint32_t i) -> Key& {
      assert(marker[i] >= 2);
      return storage.key[i];
    }
    auto Destroy(uint32_t i) -> void { storage.key[i].Key::~Key(); }
    auto MoveFrom(uint32_t i, Bucket* src, uint32_t src_index) -> void {
      new (&storage.key[i]) Key(std::move(src->storage.key[src_index]));
    }
    auto CopyFrom(uint32_t i, Bucket* src, uint32_t src_index) -> void {
      new (&storage.key[i]) Key(src->storage.key[src_index]);
    }
  };

  template <typename K>
  auto Insert(K&& k) -> std::pair<iterator, bool> {
    rep_.MaybeResize();
    auto r = rep_.FindOrInsert(std::forward<K>(k));
    const bool inserted = !r.found;
    return {iterator(r.b, rep_.limit(), r.index), inserted};
  }

  Rep rep_;
};

template <typename Key, typename Compare = std::less<Key>>
inline auto operator==(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Key, typename Compare = std::less<Key>>
inline auto operator!=(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return !(lhs == rhs);
}

template <typename Key, typename Compare = std::less<Key>>
inline auto operator<(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                      rhs.end(), Compare{});
}

template <typename Key, typename Compare = std::less<Key>>
inline auto operator<=(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return !(lhs > rhs);
}

template <typename Key, typename Compare = std::less<Key>>
inline auto operator>(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return rhs < lhs;
}

template <typename Key, typename Compare = std::less<Key>>
inline auto operator>=(const FlatSet<Key>& lhs, const FlatSet<Key>& rhs)
    -> bool {
  return !(lhs < rhs);
}

}  // namespace RG

#endif  // REGGEN_COMMON_FLAT_SET_H