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

#include "checksum.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace nucleus::protocol
{

namespace
{

[[nodiscard]] auto calculate_checksum(const std::vector<std::uint8_t> & data) -> std::uint16_t
{
  std::uint16_t sum = 0xB58C;

  for (std::size_t i = 0; i < data.size(); i += 2) {
    const std::uint8_t u = data[i];
    if (i + 1 < data.size()) {
      const std::uint8_t v = data[i + 1];
      sum += static_cast<std::uint16_t>(u | (v << 8));
    } else {
      sum += static_cast<std::uint16_t>(u << 8);
    }
    sum &= 0xFFFF;
  }

  return sum;
}

}  // namespace

[[nodiscard]] auto checksum(const std::vector<std::uint8_t> & data, std::uint16_t expected_checksum) -> bool
{
  const auto calculated_checksum = calculate_checksum(data);
  return calculated_checksum == expected_checksum;
}

}  // namespace nucleus::protocol
