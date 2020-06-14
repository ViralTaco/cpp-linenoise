#ifndef VT_COMPLETIONS_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ Completions.hpp:                                     ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_COMPLETIONS_HPP "1.1.0"

#include <string>
#include <vector>
#include <utility> // std::move
#include <initializer_list>

namespace viraltaco_ {

using namespace std::literals;

template <typename StringLikeType = std::string_view>
struct Completions : std::vector<StringLikeType> {
  using value_type = StringLikeType;
  
  using Self = Completions;
  using Type = std::vector<StringLikeType>;
  using ValueType = value_type;
  using SizeType = std::size_t;
  
public: // members
  Type completions_;

public: // inits
  constexpr Completions()  noexcept = default;
  constexpr Completions(Completions&&) noexcept = default;
  constexpr Completions(Completions const&) noexcept = default;
  
  explicit Completions(StringLikeType str) noexcept
    : completions_{ str }
  {}
  Completions(std::initializer_list<ValueType> list) :
    completions_{ list }
  {}
  explicit Completions(Type&& c) noexcept
    : completions_{ std::move(c) }
  {}
  
  template <class T> using nothrow_copy = std::is_nothrow_copy_constructible<T>;
  explicit Completions(Type const& c) noexcept (nothrow_copy<Type>::value)
    : completions_{ c }
  {}
  
};

} namespace vt = viraltaco_;

#endif
