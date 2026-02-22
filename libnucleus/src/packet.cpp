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
#include <iostream>
#include <ranges>
#include <stdexcept>
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
  const std::vector<std::uint8_t> header_data = {data.begin(), data.begin() + 10};
  const std::uint8_t header_size = header_data[1];
  const std::uint8_t series_id = header_data[2];
  const std::uint8_t family_id = header_data[3];
  const std::uint16_t data_size = (header_data[4] | (static_cast<std::uint16_t>(header_data[5]) << 8));
  const std::uint16_t data_checksum = header_data[6] | (static_cast<std::uint16_t>(header_data[7]) << 8);
  const std::uint16_t header_checksum = header_data[8] | (static_cast<std::uint16_t>(header_data[9]) << 8);

  const std::vector<std::uint8_t> packet_data = {data.begin() + header_size, data.end()};

  if (packet_data.size() != data_size) {
    throw std::invalid_argument("Data size does not match the size specified in the header.");
  }

  if (!protocol::checksum({header_data.begin(), header_data.end() - 2}, header_checksum)) {
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
  return data.size() >= calculate_packet_size({data.begin(), data.end()});
}

auto decode_packets(const std::deque<std::uint8_t> & data) -> std::tuple<std::vector<Packet>, std::size_t>
{
  if (data.empty()) {
    return {std::vector<Packet>{}, 0};
  }

  std::vector<Packet> packets;

  auto start = data.begin();
  auto iter = std::ranges::find(data, protocol::SYNC_BYTE);
  auto erase_iter = data.begin();

  while (iter != data.end()) {
    start = iter;
    auto next = std::ranges::find(std::next(iter), data.end(), protocol::SYNC_BYTE);

    std::size_t expected_size;
    try {
      expected_size = calculate_packet_size({start, next});
    }
    catch (const std::exception & e) {
      break;  // we don't have a full packet yet, so wait for more data to arrive
    }
    const std::size_t packet_distance = std::distance(start, next) - expected_size;

    if (packet_distance <= 0) {
      // we have two contiguous packets in the buffer, so we can attempt to decode multiple packets at once.
      // note that we set the packet data to be everything between the current sync byte and the next, disregarding
      // the expected size. this is because the expected size may be incorrect.
      const std::vector<std::uint8_t> packet_data(start, next);
      try {
        packets.push_back(decode_packet(packet_data));
      }
      catch (const std::exception & e) {  // NOLINT
        // decoding error - just ignore it and move on to the next packet
      }
      erase_iter = next;
    } else {
      // there is some data between the current sync byte and the next. we need to let the polling function extract
      // that ascii data before we attempt to decode any additional packets, so just extract the first packet
      const std::vector<std::uint8_t> packet_data(start, start + expected_size);
      try {
        packets.push_back(decode_packet(packet_data));
      }
      catch (const std::exception & e) {  // NOLINT
        // decoding error - just ignore it
      }
      erase_iter = start + expected_size;
      break;
    }

    iter = next;
  }

  return {packets, std::distance(data.begin(), erase_iter)};
}

}  // namespace protocol

}  // namespace nucleus
