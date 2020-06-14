#ifndef VT_VIRALNOISE_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ViralNoise.hpp:                                      ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_VIRALNOISE_HPP "1.0.0"

#ifndef _WIN32
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#else
#  include "win32/winnoise.hpp"
#endif

#include "Completions.hpp"
#include <string>
#include <vector>

namespace viraltaco_ {
using std::string_view_literals::operator""sv;

struct ViralNoise {
public:
  using Self = ViralNoise;
  
public: // members
#ifndef _WIN32
  /// In order to restore at exit.
  static termios orig_termios; // defined in unistd.h
#endif
  enum maximum : std::size_t { history_len = 100ull, line_length = 4096ull, };

  static constexpr std::string_view kUnsupportedTerms[3] {
    "dumb"sv, "cons25"sv, "emacs"sv
  };
  
  static CompletionCallback completionCallback;
  ///  For atexit() function to check if restore is needed
  bool is_raw_mode = false;
  ///  Multi line mode. Default is single line.
  bool allow_multiline = false;
  /// Register atexit just 1 time.
  bool atexit_registered = false;
  /// History length
  std::size_t history_max_len = Self::maximum::history_len;

  Completions history;
public: // inits


};
} namespace vt = viraltaco_;

#endif
