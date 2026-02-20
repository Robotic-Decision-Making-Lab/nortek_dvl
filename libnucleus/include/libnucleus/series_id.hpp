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

namespace nucleus
{

enum class SeriesId : std::uint8_t
{
  IMU_DATA = 0x82,
  MAGNETOMETER_DATA = 0x87,
  FIELD_CALIBRATION_DATA = 0x8B,
  FAST_PRESSURE_DATA = 0x96,
  STRING_DATA = 0xA0,
  ALTIMETER_DATA = 0xAA,
  BOTTOM_TRACK_DATA = 0xB4,
  WATER_TRACK_DATA = 0xBE,
  CURRENT_PROFILER_DATA = 0xC0,
  AHRS_DATA = 0xD2,
  INS_DATA = 0xDC,
};

}
