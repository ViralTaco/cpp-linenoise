#ifndef VT_COMPLETIONCALLBACK_STRINGLIST_HPP
// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ CompletionCallback.hpp:                              ┃
// ┃ Copyright (c) 2020 viraltaco_ (viraltaco@gmx.com)    ┃
// ┃ https://github.com/ViralTaco                         ┃ 
// ┃ SPDX-License-Identifier: MIT                         ┃
// ┃ <http://www.opensource.org/licenses/MIT>             ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#define VT_COMPLETIONCALLBACK_STRINGLIST_HPP "2.0.0"

#include <functional>
#include <vector>
#include <string>

namespace viraltaco_ {

using StringList = std::vector<std::string>;
using CompletionCallback = std::function<void(char const*, StringList&)>;

} namespace vt = viraltaco_;

#endif
