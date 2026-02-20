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

/// Delimiter used to separate lines in ASCII responses.
const std::string DELIMITER = "\r\n";

/// Response indicating an error in an ASCII message.
const std::string ERROR_RESPONSE = "ERROR";

/// Response indicating success in an ASCII message.
const std::string SUCCESS_RESPONSE = "OK";

/// Error response, including delimiter.
const std::string ERROR_TERMINATOR = ERROR_RESPONSE + DELIMITER;

/// Success response, including delimiter.
const std::string SUCCESS_TERMINATOR = SUCCESS_RESPONSE + DELIMITER;

/// Split a byte array that may contain one or more ASCII messages into individual responses.
///
/// This returns a vector of all ASCII responses contained in the data and an iterator pointing to the end of the last
/// response.
[[nodiscard]] auto split_responses(std::deque<std::uint8_t> & data)
  -> std::tuple<std::vector<Response>, std::deque<std::uint8_t>::iterator>;

}  // namespace nucleus::protocol
