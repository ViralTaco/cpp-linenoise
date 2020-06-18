#ifndef VT_COMPLETIONS_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ Completions.hpp:                                     ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_COMPLETIONS_HPP "2.3.0"

#include <string>
#include <utility>
#include <vector>
#include <utility> // std::move
#include <algorithm>
#include <functional>
#include <type_traits>
#include <initializer_list>

#include "exceptions/NullCallbackPointerError.hpp"

namespace viraltaco_ {
using namespace std::literals;

template <typename StringLike = std::string>
struct Completions : std::vector<std::string> {
public:
  /// Fully qualified class name
  static constexpr auto class_name = "viraltaco_::Completions";
    
public:
  using value_type = std::string ;
  
  using Self = Completions;
  using SizeType = std::size_t;
  using ValueType = value_type;
  using Type = std::vector<ValueType>;
  using List = Type;
  
  /// callback function definition
  using CallbackType = std::function<ValueType(ValueType, List)>;

public: // members
  Type completions;
  /// Callback for completions. By default there is no such function.
  CallbackType callback = Self::default_callback;

public: // inits
  constexpr Completions() noexcept = default;
  constexpr Completions(Completions&&) noexcept = default;
  constexpr Completions(Completions const&) noexcept = default;
  
  explicit Completions(ValueType str) noexcept
    : completions{ std::move(str) }
  {}
  Completions(std::initializer_list<ValueType> list)
    : completions{ list }
  {}
  explicit Completions(Type&& c) noexcept
    : completions{ std::move(c) }
  {}
  
  template <class T> using nothrow_move = std::is_nothrow_move_constructible<T>;
  explicit Completions(Type c) noexcept (nothrow_move<Type>::value)
    : completions{ std::move(c ) }
  {}

public: // setters
  /**
   * This method takes a {@code Self::CallbackType}, 
   * and sets this->callback to it.
   *
   * @param callback_function a std::function callback function. 
   * @throws NullCallbackPointerError if the arg is null 
   */
  void set_callback(CallbackType const& callback_function)
  noexcept (NullCallbackPointerError::value) {
    if (callback_function == nullptr) {
      throw NullCallbackPointerError(
        class_name + "::set_callback(CallbackType*) was passed null."s
      );
    } else {
      this->callback = callback_function;
    }
  }
  
  
public: // instance methods
  auto operator << (StringLike const& str) {
    this->completions.push_back(str);
    return this;
  }
  
  auto add(StringLike const& str) {
    return this->operator<<(str);
  }
  
public: // class methods
  /*
   * TODO This method is slow
   * because I used std::find_if with std::string::starts_with in the predicate.
   * Find a way to make it faster. (Maybe using a map would be smarter)
   */
  static std::string default_callback(const std::string s, List const& list) {
    const auto found = std::find_if(list.begin(), list.end(),
      [s] (const auto x) { return x.starts_with(s); }
    );
  
    return (found != list.end()) ? *found : s;
  }
  
};

} namespace vt = viraltaco_;

#endif
