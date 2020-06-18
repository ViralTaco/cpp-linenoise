#ifndef VT_CPP_LINENOISE_LINENOISE_MODEL_EXCEPTIONS_IOEXCEPTION_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ IOException.hpp:                                     ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_CPP_LINENOISE_LINENOISE_MODEL_EXCEPTIONS_IOEXCEPTION_HPP "1.0.0"

#include <exception>
#include <string>

namespace viraltaco_ {
class IOException : public std::exception {
public:
  using Self = IOException;
  static constexpr auto value = false;
  
private: // members
  std::string message_;

public: // inits
  template <typename StringType>
  explicit IOException(StringType message) noexcept
    : message_{ message }
  {}

public: // instance methods
  [[nodiscard]] char const* what() const noexcept override {
    return this->message_.c_str();
  }
};
} namespace vt = viraltaco_;
#endif //VT_CPP_LINENOISE_LINENOISE_MODEL_EXCEPTIONS_IOEXCEPTION_HPP
