#ifndef VT_TERMINAL_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ Terminal.hpp:                                        ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_TERMINAL_HPP "1.0.0"

#include <unistd.h>
#include <cstdint>
#include <string>
#include <sstream>
#include <ios>


namespace viraltaco_ {
class Terminal {
public:
  using Self = Terminal;
  using FileDescriptor = decltype (STDIN_FILENO);
  using Index = std::size_t;
  using Buffer = char*;
  
private: // members
  std::ifstream in;
  std::ofstream out;

  FileDescriptor ifd = STDIN_FILENO;     ///  Terminal stdin file descriptor.
  FileDescriptor ofd = STDOUT_FILENO;    ///  Terminal stdout file descriptor.
  Buffer buf;             ///  Edited line buffer.
  std::size_t buflen;     ///  Edited line buffer size.
  std::string prompt;     ///  Prompt to display.
  std::size_t pos;        ///  Current cursor position.
  std::size_t oldcolpos;  ///  Previous refresh cursor column position.
  std::size_t len;        ///  Current edited line length.
  std::size_t cols;       ///  Number of columns in terminal.
  std::size_t maxrows = 1u;    ///  Maximum num of rows used so far (multiline mode)
  Index history_index = 0u;    ///  The history index we are currently editing.

public: // inits
  Terminal(std::string prompt)
    : prompt{ std::move(prompt) }
  {}
    
public: // class methods
  
  
};
} namespace vt = viraltaco_;
#endif
