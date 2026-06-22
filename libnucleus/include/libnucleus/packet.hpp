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
#include <cstdio>
#include <deque>
#include <vector>

#include "libnucleus/family_id.hpp"
#include "libnucleus/series_id.hpp"

namespace nucleus
{

template <typename T>
struct Deserializer
{
  static void from_data(const std::vector<std::uint8_t> & /*data*/, T & /*value*/)
  {
    // TODO(evan-palmer): convert to `std::println` once supported by the compiler
    std::printf("No deserializer defined for this type.\n");
  }
};

class Packet
{
public:
  /// Create a new "binary data" packet given the series ID and data.
  Packet(SeriesId series_id, FamilyId family_id, std::vector<std::uint8_t> data);

  [[nodiscard]] auto series_id() const -> SeriesId;

  [[nodiscard]] auto family_id() const -> FamilyId;

  [[nodiscard]] auto data() const -> std::vector<std::uint8_t>;

  [[nodiscard]] auto data_size() const -> std::size_t;

  template <typename T>
  [[nodiscard]] auto get() const -> T
  {
    T value;
    Deserializer<T>::from_data(data_, value);
    return value;
  }

private:
  SeriesId series_id_;
  FamilyId family_id_;
  std::vector<std::uint8_t> data_;
};

namespace protocol
{
/// Sync byte used to identify the start of a Nortek Nucleus packet.
const std::uint8_t SYNC_BYTE = 0xA5;

/// Check if the given data might contain a valid packet.
[[nodiscard]] auto might_contain_packet(const std::deque<std::uint8_t> & data) -> bool;

/// Decode all packets from a byte array. This returns a vector of all decoded packets and an iterator pointing to the
/// end of the last decoded packet. This can be used to remove the decoded data from the buffer.
[[nodiscard]] auto decode_packets(const std::deque<std::uint8_t> & data)
  -> std::tuple<std::vector<Packet>, std::size_t>;

}  // namespace protocol

}  // namespace nucleus
