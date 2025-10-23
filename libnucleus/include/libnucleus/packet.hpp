// Copyright 2025, Evan Palmer
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

#include "libnortek/series_id.hpp"

namespace nucleus
{

class Packet
{
public:
  /// Create a new "binary data" packet given the series ID and data.
  Packet(SeriesId id, std::vector<std::uint8_t> data);

  [[nodiscard]] auto series_id() const -> SeriesId;

  [[nodiscard]] auto data() const -> const std::vector<std::uint8_t> &;

  [[nodiscard]] auto data_size() const -> std::size_t;

  [[nodiscard]] auto pop_front(std::size_t size) -> std::vector<std::uint8_t>;

  [[nodiscard]] auto pop_back(std::size_t size) -> std::vector<std::uint8_t>;

  template <typename T>
  [[nodiscard]] auto get() -> T;

private:
  SeriesId series_id_;
  std::vector<std::uint8_t> data_;
};

namespace protocol
{
/// Sync byte used to identify the start of a Nortek Nucleus packet.
const std::uint8_t SYNC_BYTE = 0xA5;

template <typename T>
[[nodiscard]] inline auto deserialize(const Packet & packet) -> T;

}  // namespace protocol

}  // namespace nucleus