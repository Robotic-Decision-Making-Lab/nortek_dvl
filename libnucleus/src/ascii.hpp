// Copyright 2026, Evan Palmer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "libnucleus/response.hpp"

namespace nucleus::protocol
{

const std::string DELIMITER = "\r\n";

const std::string ERROR_RESPONSE = "ERROR";

const std::string SUCCESS_RESPONSE = "OK";

const std::string ERROR_TERMINATOR = ERROR_RESPONSE + DELIMITER;

const std::string SUCCESS_TERMINATOR = SUCCESS_RESPONSE + DELIMITER;

[[nodiscard]] auto split_responses(std::deque<std::uint8_t> & data)
  -> std::tuple<std::vector<CommandResponse>, std::deque<std::uint8_t>::iterator>;

}  // namespace nucleus::protocol
