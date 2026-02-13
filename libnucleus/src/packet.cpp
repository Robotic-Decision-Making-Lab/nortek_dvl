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
#include <vector>

#include "checksum.hpp"

namespace nucleus
{

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

auto decode_packet(const std::vector<std::uint8_t> & data) -> Packet
{
  if (data.empty()) {
    throw std::invalid_argument("Cannot decode an empty byte stream.");
  }

  const std::uint8_t header_size = data[1];
  if (data.size() < header_size) {
    throw std::invalid_argument("Data size is smaller than the specified header size.");
  }

  const std::vector<std::uint8_t> header_data = {data.begin(), data.begin() + header_size};
  const std::vector<std::uint8_t> packet_data = {data.begin() + header_size, data.end()};

  const std::uint8_t series_id = header_data[2];
  const std::uint8_t family_id = header_data[3];
  const std::uint16_t data_size = (header_data[4] | (static_cast<std::uint16_t>(header_data[5]) << 8));
  const std::uint16_t data_checksum = header_data[6] | (static_cast<std::uint16_t>(header_data[7]) << 8);
  const std::uint16_t header_checksum = header_data[8] | (static_cast<std::uint16_t>(header_data[9]) << 8);

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

auto decode_packets(const std::vector<std::uint8_t> & data) -> std::vector<Packet>
{
  if (data.empty()) {
    throw std::invalid_argument("Cannot decode an empty buffer.");
  }

  std::vector<Packet> packets;

  auto start = data.begin();
  auto iter = std::find(start, data.end(), protocol::SYNC_BYTE);

  while (iter != data.end()) {
    start = iter;
    iter = std::find(start + 1, data.end(), protocol::SYNC_BYTE);
    const std::vector<std::uint8_t> packet_data(start, iter);

    try {
      const Packet packet = decode_packet(packet_data);
      packets.push_back(packet);
    }
    catch (const std::exception & e) {  // NOLINT(bugprone-empty-catch)
      // skip invalid packets (usually just incomplete packets) - we don't log here to avoid spamming the user
    }
  }

  return packets;
}

}  // namespace protocol

}  // namespace nucleus
