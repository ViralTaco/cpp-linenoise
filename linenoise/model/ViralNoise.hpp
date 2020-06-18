#ifndef VT_VIRALNOISE_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ViralNoise.hpp:                                      ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_VIRALNOISE_HPP "2.0.0"

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <cstdlib> // std::getenv

#include "Completions.hpp"
#include <string>
#include <vector>
#include <array>

namespace viraltaco_ {
using namespace std::literals;

template <typename StringLikeType> struct ViralNoise {
public:
  /// Fully qualified class name
  static constexpr auto class_name = "viraltaco_::ViralNoise";
  using Self = ViralNoise;
  using CallbackType = typename Completions<StringLikeType>::CallbackType;
  
public: // members
  /// In order to restore at exit.
  static termios orig_termios; // defined in unistd.h
  enum maximum : std::size_t {
    history_len = 100ull,
    line_length = 4096ull,
  };
  
  static constexpr std::array<char const*, 4> kUnsupportedTerms {
    "dumb",
    "cons25",
    "emacs",
    nullptr, // Unknown therefore unsupported.
  };
  /// Multi line mode. Default is single line.
  bool allow_multiline = false;
  /// History length
  std::size_t history_max_len = Self::maximum::history_len;
  /// For unsupported terminals
  bool is_raw_mode;
  Completions<StringLikeType> history;
  StringLikeType prompt;
  /// Callback function.
  CallbackType completion_callback;
  
public: // inits
  /// No default constructor.
  ViralNoise() = delete;
  ViralNoise(Self&&) noexcept = default;
  ViralNoise(Self const&) noexcept = default;
  
  
  /**
   * This constructor takes a prompt to be displayed on the terminal
   * and a {@code Completions<>} to add to the otherwise empty
   * history variable (of the same type) since both behave identically.
   *
   * @param prompt the terminal prompt
   * @param completions the completion strings
   */
  ViralNoise(StringLikeType prompt,
             Completions<StringLikeType> completions)
    : is_raw_mode{ Self::is_unsupported_term() }
    , history{ completions }
    , prompt{ prompt }
    , completion_callback{ history.callback }
  {}
  
  explicit ViralNoise(StringLikeType prompt)
    : is_raw_mode{ Self::is_unsupported_term() }
    , prompt { prompt }
  {}

public: // destructors
  ~ViralNoise() noexcept {
    if (is_raw_mode) {
      is_raw_mode = (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) != -1);
    }
  }

protected: // class methods
  static bool is_unsupported_term() noexcept {
    const auto env_term = std::getenv("TERM");
    /// Return true if the terminal is unsupported
    return std::any_of(kUnsupportedTerms.begin(), kUnsupportedTerms.end(),
      [env_term] (const auto term) noexcept -> bool {
        return term == env_term;
      }
    );
  }
  
public: // instance methods
  
  template <typename T> Self& operator >> (T& arg_ref) {
  
  }

};
} namespace vt = viraltaco_;

#endif
