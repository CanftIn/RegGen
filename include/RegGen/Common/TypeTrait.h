#ifndef REGGEN_COMMON_TYPE_TRAIT_H
#define REGGEN_COMMON_TYPE_TRAIT_H

#include <iterator>
#include <type_traits>

namespace RG {

template <typename T>
struct AlwaysTrue : std::true_type {};

template <typename T>
struct AlwaysFalse : std::false_type {};

namespace Internal {

struct TypeChecker {};

template <typename... Ts>
inline constexpr auto IsTypeChecker() -> bool {
  return std::conjunction_v<std::is_base_of<TypeChecker, Ts>...>;
}

template <typename... Ts>
using EnsureTypeChecker = std::enable_if_t<IsTypeChecker<Ts...>()>;

template <typename T1, typename T2>
struct And : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return T1::template Evaluate<T>() && T2::template Evaluate<T>();
  }
};

template <typename T1, typename T2>
struct Or : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return T1::template Evaluate<T>() || T2::template Evaluate<T>();
  }
};

template <typename P>
struct Not : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return !P::template Evaluate<T>();
  }
};

template <template <typename> typename Pred>
struct GenericTypeChecker : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    static_assert(std::is_same_v<typename Pred<T>::value_type, bool>);
    return Pred<T>::value;
  }
};

}  // namespace Internal

template <typename T1, typename T2,
          typename = Internal::EnsureTypeChecker<T1, T2>>
inline constexpr auto operator&&(T1 /*unused*/, T2 /*unused*/) {
  return Internal::And<T1, T2>{};
}

template <typename T1, typename T2,
          typename = Internal::EnsureTypeChecker<T1, T2>>
inline constexpr auto operator||(T1 /*unused*/, T2 /*unused*/) {
  return Internal::Or<T1, T2>{};
}

template <typename P, typename = Internal::EnsureTypeChecker<P>>
inline constexpr auto operator!(P /*unused*/) {
  return Internal::Not<P>{};
}

template <template <typename> typename Pred>
static constexpr auto generic_type_checker =
    Internal::GenericTypeChecker<Pred>{};

template <typename T, typename P, typename = Internal::EnsureTypeChecker<P>>
static constexpr auto Constraint(P /*unused*/) -> bool {
  return P::template Evaluate<T>();
}

static constexpr auto is_void = generic_type_checker<std::is_void>;
static constexpr auto is_null_pointer =
    generic_type_checker<std::is_null_pointer>;
static constexpr auto is_integral = generic_type_checker<std::is_integral>;
static constexpr auto is_floating_point =
    generic_type_checker<std::is_floating_point>;
static constexpr auto is_array = generic_type_checker<std::is_array>;
static constexpr auto is_enum = generic_type_checker<std::is_enum>;
static constexpr auto is_union = generic_type_checker<std::is_union>;
static constexpr auto is_class = generic_type_checker<std::is_class>;
static constexpr auto is_function = generic_type_checker<std::is_function>;
static constexpr auto is_pointer = generic_type_checker<std::is_pointer>;
static constexpr auto is_lvalue_reference =
    generic_type_checker<std::is_lvalue_reference>;
static constexpr auto is_rvalue_reference =
    generic_type_checker<std::is_rvalue_reference>;
static constexpr auto is_member_object_pointer =
    generic_type_checker<std::is_member_object_pointer>;
static constexpr auto is_member_function_pointer =
    generic_type_checker<std::is_member_function_pointer>;

static constexpr auto is_fundamental =
    generic_type_checker<std::is_fundamental>;
static constexpr auto is_arithmetic = generic_type_checker<std::is_arithmetic>;
static constexpr auto is_scalar = generic_type_checker<std::is_scalar>;
static constexpr auto is_object = generic_type_checker<std::is_object>;
static constexpr auto is_compound = generic_type_checker<std::is_compound>;
static constexpr auto is_reference = generic_type_checker<std::is_reference>;
static constexpr auto is_member_pointer =
    generic_type_checker<std::is_member_pointer>;

static constexpr auto is_const = generic_type_checker<std::is_const>;
static constexpr auto is_volatile = generic_type_checker<std::is_volatile>;
static constexpr auto is_trivial = generic_type_checker<std::is_trivial>;
static constexpr auto is_trivially_copyable =
    generic_type_checker<std::is_trivially_copyable>;
static constexpr auto is_standard_layout =
    generic_type_checker<std::is_standard_layout>;
static constexpr auto is_pod = generic_type_checker<std::is_pod>;
static constexpr auto is_empty = generic_type_checker<std::is_empty>;
static constexpr auto is_polymorphic =
    generic_type_checker<std::is_polymorphic>;
static constexpr auto is_abstract = generic_type_checker<std::is_abstract>;
static constexpr auto is_final = generic_type_checker<std::is_final>;
static constexpr auto is_signed = generic_type_checker<std::is_signed>;
static constexpr auto is_unsigned = generic_type_checker<std::is_unsigned>;

namespace Internal {

template <typename... Us>
struct SameToAny : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::conjunction_v<std::is_same<T, Us>...>;
  }
};

template <typename... BasePack>
struct DeriveFromAny : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::conjunction_v<std::is_base_of<BasePack, T>...>;
  }
};

template <typename... Us>
struct ConvertibleToAny : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::conjunction_v<std::is_convertible<T, Us>...>;
  }
};

template <typename... Args>
struct Invocable : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::is_invocable_v<T, Args...>;
  }
};

template <typename... Args>
struct InvocableR : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::is_invocable_r_v<T, Args...>;
  }
};

}  // namespace Internal

template <typename U>
static constexpr auto same_to = Internal::SameToAny<U>();
template <typename... Us>
static constexpr auto same_to_any = Internal::SameToAny<Us...>();
template <typename Base>
static constexpr auto derive_from = Internal::DeriveFromAny<Base>();
template <typename... BasePack>
static constexpr auto derived_from_any = Internal::DeriveFromAny<BasePack...>();
template <typename U>
static constexpr auto convertible_to = Internal::ConvertibleToAny<U>();
template <typename... Us>
static constexpr auto convertible_to_any = Internal::ConvertibleToAny<Us...>();
template <typename... Args>
static constexpr auto invocable = Internal::Invocable<Args...>();
template <typename... Args>
static constexpr auto invocable_r = Internal::InvocableR<Args...>();

namespace Internal {

template <typename U>
struct SimilarTo : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::is_same_v<std::decay_t<U>, std::decay_t<T>>;
  }
};

// test via if category_tag could be found
template <typename T, typename = void>
struct IsIteratorInternal : public std::false_type {};
template <typename T>
struct IsIteratorInternal<
    T, std::void_t<typename std::iterator_traits<T>::iterator_category>>
    : public std::true_type {};

struct IsIterator : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return IsIteratorInternal<T>::value;
  }
};

template <typename U>
struct IsIteratorOf : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    using UnderlyingType = typename std::iterator_traits<T>::value_type;

    return std::is_same_v<UnderlyingType, U>;
  }
};

template <typename U>
struct IsEnumOf : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    if constexpr (std::is_enum_v<T>) {
      return std::is_same_v<std::underlying_type_t<T>, U>;
    } else {
      return false;
    }
  }
};

template <typename U>
struct IsReferenceOf : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::is_reference_v<T> &&
           std::is_same_v<std::remove_reference_t<T>, U>;
  }
};

template <typename U>
struct IsPointerOf : public TypeChecker {
  template <typename T>
  static constexpr auto Evaluate() -> bool {
    return std::is_pointer_v<T> && std::is_same_v<std::remove_pointer_t<T>, U>;
  }
};

}  // namespace Internal

template <typename U>
static constexpr auto similar_to = Internal::SimilarTo<U>{};

static constexpr auto is_iterator = Internal::IsIterator{};

template <typename U>
static constexpr auto is_iterator_of = Internal::IsIteratorOf<U>{};

template <typename U>
static constexpr auto is_enum_of = Internal::IsEnumOf<U>{};

template <typename U>
static constexpr auto is_reference_of = Internal::IsReferenceOf<U>{};

template <typename U>
static constexpr auto is_pointer_of = Internal::IsPointerOf<U>{};

}  // namespace RG

using ::RG::operator&&;
using ::RG::operator||;
using ::RG::operator!;

#endif  // REGGEN_COMMON_TYPE_TRAIT_H