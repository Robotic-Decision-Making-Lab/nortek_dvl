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

#include "libnucleus/response.hpp"

#include <cstdint>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "ascii.hpp"

namespace nucleus::protocol
{

auto decode_responses(const std::deque<std::uint8_t> & data) -> std::tuple<std::vector<Response>, std::size_t>
{
  if (data.empty()) {
    return {std::vector<Response>{}, 0};
  }

  const std::string data_str(data.begin(), data.end());

  std::size_t last_index = 0;
  auto update_last_index = [&](std::size_t position, std::size_t term_size) -> void {
    if (position != std::string::npos) {
      const std::size_t index = position + term_size;
      last_index = std::max(last_index, index);
    }
  };

  update_last_index(data_str.rfind(SUCCESS_TERMINATOR), SUCCESS_TERMINATOR.size());
  update_last_index(data_str.rfind(ERROR_TERMINATOR), ERROR_TERMINATOR.size());

  std::vector<Response> responses;

  const auto valid_messages = std::string_view(data_str.begin(), data_str.begin() + last_index);
  auto lines = valid_messages | std::views::split(std::string_view(DELIMITER));

  std::string message;
  for (const auto & line : lines) {
    const auto line_view = std::string_view(line);

    if (line_view.contains(ERROR_RESPONSE)) {
      responses.push_back({.success = false, .message = message});
      message = "";
    } else if (line_view.contains(SUCCESS_RESPONSE)) {
      responses.push_back({.success = true, .message = message});
      message = "";
    } else {
      message.reserve(message.size() + line_view.size() + DELIMITER.size());
      message += line_view;
      message += DELIMITER;
    }
  }

  return {responses, last_index};
}

}  // namespace nucleus::protocol
