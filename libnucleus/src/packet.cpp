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

#include "libnucleus/packet.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "checksum.hpp"

namespace nucleus
{

namespace
{

auto calculate_packet_size(const std::vector<std::uint8_t> & data) -> std::size_t
{
  if (data.size() < 6) {
    throw std::invalid_argument("Data size is too small to contain a valid packet.");
  }
  const std::uint8_t header_size = data[1];
  const std::uint16_t data_size = (data[4] | (static_cast<std::uint16_t>(data[5]) << 8));
  return header_size + data_size;
}

auto decode_packet(const std::vector<std::uint8_t> & data) -> Packet
{
  if (data.size() < 10) {
    throw std::invalid_argument("Data size is too small to contain a valid packet.");
  }

  // don't ask me why nortek does this, just accept it and move on.
  const std::vector<std::uint8_t> header_data(data.begin(), data.begin() + 10);
  const std::uint8_t header_size = header_data[1];
  const std::uint8_t series_id = header_data[2];
  const std::uint8_t family_id = header_data[3];
  const std::uint16_t data_size = (header_data[4] | (static_cast<std::uint16_t>(header_data[5]) << 8));
  const std::uint16_t data_checksum = header_data[6] | (static_cast<std::uint16_t>(header_data[7]) << 8);
  const std::uint16_t header_checksum = header_data[8] | (static_cast<std::uint16_t>(header_data[9]) << 8);

  if (header_size > data.size()) {
    throw std::invalid_argument("Header size exceeds total data size.");
  }
  const std::vector<std::uint8_t> packet_data(data.begin() + header_size, data.end());

  if (packet_data.size() != data_size) {
    throw std::invalid_argument("Data size does not match the size specified in the header.");
  }

  if (!protocol::checksum(std::vector<std::uint8_t>(header_data.begin(), header_data.end() - 2), header_checksum)) {
    throw std::invalid_argument("Header checksum does not match the calculated checksum.");
  }

  if (!protocol::checksum(packet_data, data_checksum)) {
    throw std::invalid_argument("Data checksum does not match the calculated checksum.");
  }

  return {static_cast<SeriesId>(series_id), static_cast<FamilyId>(family_id), packet_data};
}

}  // namespace

Packet::Packet(SeriesId series_id, FamilyId family_id, std::vector<std::uint8_t> data)
: series_id_(series_id),
  family_id_(family_id),
  data_(std::move(data))
{
}

auto Packet::series_id() const -> SeriesId { return series_id_; }

auto Packet::family_id() const -> FamilyId { return family_id_; }

auto Packet::data() const -> std::vector<std::uint8_t> { return data_; }

auto Packet::data_size() const -> std::size_t { return data_.size(); }

namespace protocol
{

auto might_contain_packet(const std::deque<std::uint8_t> & data) -> bool
{
  // the absolute minimum amount of data that we need is 6 bytes; this allows us to check the header size and data size
  if (data.size() < 6) {
    return false;
  }
  return data.size() >= calculate_packet_size(std::vector<std::uint8_t>(data.begin(), data.end()));
}

auto decode_packets(const std::deque<std::uint8_t> & data) -> std::tuple<std::vector<Packet>, std::size_t>
{
  if (data.empty()) {
    return {std::vector<Packet>{}, 0};
  }

  std::vector<Packet> packets;

  auto erase_iter = std::ranges::find(data, protocol::SYNC_BYTE);
  auto sync = erase_iter;

  // the minimum number of bytes needed to compute the packet size from the header
  const std::size_t min_header_bytes = 6;

  while (sync != data.end()) {
    if (std::cmp_less(std::distance(sync, data.end()), min_header_bytes)) {
      break;
    }

    std::size_t expected_size;
    try {
      expected_size = calculate_packet_size(std::vector<std::uint8_t>(sync, data.end()));
    }
    catch (const std::exception & e) {
      break;
    }

    if (std::cmp_greater(expected_size, std::distance(sync, data.end()))) {
      break;
    }

    const std::vector<std::uint8_t> packet_data(sync, sync + expected_size);
    try {
      packets.push_back(decode_packet(packet_data));
      erase_iter = sync + expected_size;
      sync = std::ranges::find(erase_iter, data.end(), protocol::SYNC_BYTE);
    }
    catch (const std::exception & e) {  // NOLINT
      sync = std::ranges::find(std::next(sync), data.end(), protocol::SYNC_BYTE);
    }
  }

  return {packets, std::distance(data.begin(), erase_iter)};
}

}  // namespace protocol

}  // namespace nucleus
