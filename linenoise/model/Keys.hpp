#ifndef VT_KEYS_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ Keys.hpp:                                            ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_KEYS_HPP "1.0.0"

#include <cstdint>

namespace viraltaco_ {
enum Keys : std::uint8_t {
  null = 0, 
  ctrl_a = 1,     
  ctrl_b = 2,     
  ctrl_c = 3,     
  ctrl_d = 4,     
  ctrl_e = 5,     
  ctrl_f = 6,     
  ctrl_h = 8,     
  tab = 9,        
  ctrl_k = 11,    
  ctrl_l = 12,    
  enter = 13,     
  ctrl_n = 14,    
  ctrl_p = 16,    
  ctrl_t = 20,    
  ctrl_u = 21,    
  ctrl_w = 23,    
  esc = 27,       
  backspace = 127 
};

} namespace vt = viraltaco_;
#endif