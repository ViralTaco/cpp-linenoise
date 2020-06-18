#ifndef VT_CPP_LINENOISE_LINENOISE_MODEL_RAWREADER_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ RawReader.hpp:                                       ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_CPP_LINENOISE_LINENOISE_MODEL_RAWREADER_HPP "1.0.0"

#include "exceptions/IOException.hpp"

#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <termios.h>
#include <unistd.h>

namespace viraltaco_ {
/**
 * This class reads from a filedescriptor in raw mode.
 * By default it reads from stdin.
 */
template <class CharType = unsigned char> class RawReader {
public:
  static constexpr auto class_name = "RawReader";
  
  using Self = RawReader;
  using Type = CharType;
  using FileDescriptor = int;

protected:
  using Char = CharType;
  using FD = FileDescriptor;
  
private: // members
  FD fd_;
  bool normal_mode_;
  termios default_term_state_;
  termios term_state_;
  Char last_ = static_cast<Char> (0);
  
public: // inits
  constexpr RawReader() noexcept = delete;
  constexpr RawReader(Self&&) noexcept = default;
  constexpr RawReader(Self const&) = default;
  
  explicit RawReader(FD fd = STDIN_FILENO) noexcept
    : fd_{ fd }
    , normal_mode_{ (fd_ != STDIN_FILENO) }
    , default_term_state_{ Self::get_term_state(fd_) }
    , term_state_{ default_term_state_ }
  {
    if constexpr (not normal_mode_) {
      enable_raw_mode();
      std::atexit(&Self::reset_term_state);
    }
  }
  
  ~RawReader() noexcept {
    if (not normal_mode_) {
      reset_term_state();
    }
  }

public: // class methods
protected: // class methods
  static termios get_term_state(const FD fd) noexcept {
    // This is defined in <termios.h> to understand what it does follow the link
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/tcgetattr.html
    termios state{};
    tcgetattr(fd, &state);
    return state;
  }

  static void reset_term_state() noexcept {
    const auto default_state =  Self::get_term_state(STDIN_FILENO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &default_state);
  }
  
public: // instance methods
  void disable_raw_mode() noexcept {
    // This is defined in <termios.h> to understand what it does follow the link
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/tcsetattr.html
    tcsetattr(fd_, TCSAFLUSH, &default_term_state_);
  }
  
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
  void enable_raw_mode() noexcept {
    // Reminder operator ~ (compl) is bitwise not.
    // ~BRKINT  : Disable signal interrupt on break.
    // ~ICRNL   : Disable mapping of CR (\r) to NL (\n) on input.
    // ~INPCK   : Disable input parity check.
    // ~ISTRIP  : Disable "Strip character" (8th bit of each input byte)
    // ~IXON    : Disable "start/stop output control" (crtl-s and ctrl-q)
    term_state_.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // ~OPOST   : Disable post-process output.
    term_state_.c_oflag &= ~(OPOST);
    // CS8      : Set character size to 8 bit.
    term_state_.c_cflag |= (CS8);
    // ~ECHO    : Disable echo.
    // ~ICANON  : Disable canonical input.
    // ~IEXTEN  : Disable extended input character processing.
    // ~ISIG    : Disable signals.
    term_state_.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // VMIN     : Minimum amount of chars before read() returns.
    term_state_.c_cc[VMIN] = 0;
    // VTIME    : Timeout before read() returns (in 10ths of seconds)
    term_state_.c_cc[VTIME] = 1; // 100ms
    
    tcsetattr(fd_, TCSAFLUSH, &term_state_);
  }
#pragma clang diagnostic pop

  Char read() noexcept (IOException::value) {
    using namespace std::literals;
    
    Char c = static_cast<Char> (0);
    int nread;
    // Loop until we read a character.
    while ((nread = read(fd_, &c, 1)) != 1) {
      // EAGAIN : No data read, try again.
      if (nread != -1 or errno == EAGAIN) { continue; }
      
      throw IOException(class_name + ".read(): "s + std::strerror(errno));
    }
    return c;
  }

};
} namespace vt = viraltaco_;

#endif //CPP_LINENOISE_LINENOISE_MODEL_RAWREADER_HPP
