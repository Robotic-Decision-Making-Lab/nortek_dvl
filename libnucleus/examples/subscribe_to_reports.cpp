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

#include <iostream>

#include "libnucleus/client.hpp"
#include "libnucleus/report.hpp"
#include "libnucleus/series_id.hpp"

auto main() -> int
{
  nucleus::NucleusClient client("192.168.2.201", "nortek");
  auto start_future = client.start_measurement();

  client.subscribe<nucleus::BottomTrackReport>([](const nucleus::BottomTrackReport & report) -> void {
    std::cout << std::format("timestamp={}, vx={}, vy={}, vz={}\n", report.timestamp, report.vx, report.vy, report.vz);
    std::cout << std::format("velocity_valid={}\n", report.velocity_valid);
  });

  std::this_thread::sleep_for(std::chrono::seconds(10));

  auto stop_future = client.stop_measurement();

  return 0;
}
