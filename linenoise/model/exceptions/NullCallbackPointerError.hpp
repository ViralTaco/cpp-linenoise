#ifndef VT_NULLCALLBACKPOINTERERROR_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ NullCallbackFunctionPointerException.hpp:                                        ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_NULLCALLBACKPOINTERERROR_HPP "1.0.1"

#include <exception>
#include <string>

namespace viraltaco_ {
class NullCallbackPointerError : public std::exception {
public:
  using Self = NullCallbackPointerError;
  static constexpr auto value = false;
  
private: // members
  std::string message_;

public: // inits
  template <typename StringType>
  explicit NullCallbackPointerError(StringType message) noexcept
    : message_{ message }
  {}

public: // instance methods
  [[nodiscard]] char const* what() const noexcept override {
    return this->message_.c_str();
  }
};
} namespace vt = viraltaco_;
#endif
