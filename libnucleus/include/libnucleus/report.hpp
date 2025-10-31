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

#include <Eigen/Dense>
#include <chrono>
#include <cstdint>
#include <ranges>

#include "libnucleus/packet.hpp"

namespace nucleus
{

struct CommonData
{
  // Data format version.
  std::uint8_t version;

  // Number of bytes from start of record to start of non-common data fields.
  std::uint8_t data_offset;

  // Timestamp of the report.
  //
  // If posix_time is true, this is the POSIX time in nanoseconds since epoch. Otherwise, it is the time since the START
  // command was issued.
  std::chrono::nanoseconds timestamp;

  // Time since the last timestamp (nanoseconds).
  std::chrono::nanoseconds time_since_stamp;

  // Whether the timestamp is POSIX time or time since START command.
  bool posix_time;
};

struct AHRSReport : CommonData
{
  // Instrument serial number.
  std::uint32_t serial_number;

  // AHRS operation mode:
  //   - 0: Field calibration
  //   - 2: Regular AHRS mode
  std::uint8_t operation_mode;

  // Quality measure of AHRS (0 when not running).
  float fom;

  // Quality measure of field calibration (outputs 0 if hard iron is not estimated).
  float fom_field_calibration;

  // Orientation of the instrument.
  Eigen::Quaternionf orientation;

  // Magnetic declination (deg).
  float declination;

  // Depth below sea surface, estimated from pressure (m).
  float depth;
};

struct INSReport : AHRSReport
{
  // Figure of merit (0 when not running).
  float fom;

  // Course over ground (deg).
  float course_over_ground;

  // Temperature (deg C).
  float temperature;

  // Pressure (Bar).
  float pressure;

  // Hight above sea floor, as measured by the altimeter (m).
  float altitude;

  // Latitude (deg).
  double latitude;

  // Longitude (deg).
  double longitude;

  // Whether or not the latitude and longitude are valid.
  bool lat_lon_valid;

  // Relative x position (m).
  float x;

  // Relative y position (m).
  float y;

  // Relative z position (m).
  float z;

  // Velocity along the sensor x axis (m/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float vx;

  // Velocity along the sensor y axis (m/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float vy;

  // Velocity along the sensor z axis (m/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float vz;

  // Angular velocity about the sensor x axis (rad/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float wx;

  // Angular velocity about the sensor y axis (rad/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float wy;

  // Angular velocity about the sensor z axis (rad/s).
  //
  // If an orientation offset is applied to the sensor, this velocity is measured in the offset frame.
  float wz;

  // Ground speed (m/s).
  float ground_speed;
};

struct IMUReport : CommonData
{
  // Linear acceleration along the x axis (m/s^2).
  float ax;

  // Linear acceleration along the y axis (m/s^2).
  float ay;

  // Linear acceleration along the z axis (m/s^2).
  float az;

  // Angular velocity about the sensor x axis (rad/s).
  float wx;

  // Angular velocity about the sensor y axis (rad/s).
  float wy;

  // Angular velocity about the sensor z axis (rad/s).
  float wz;

  // Temperature (deg C).
  float temperature;

  // IMU data is valid.
  bool imu_valid;
};

struct MagnetometerReport : CommonData
{
  // Magnetic field along the x axis (gauss).
  float mx;

  // Magnetic field along the y axis (gauss).
  float my;

  // Magnetic field along the z axis (gauss).
  float mz;

  // Whether or not the magnetometer data is compensated for hard iron effects.
  bool is_compensated_for_hard_iron;
};

struct AltimeterReport : CommonData
{
  // Instrument serial number.
  std::uint32_t serial_number;

  // Measured or configured speed of sound (m/s).
  float speed_of_sound;

  // Altitude (m).
  float distance;

  // Pressure (Bar).
  float pressure;

  // Temperature (deg C).
  float temperature;

  // Whether or not the altimeter distance is valid.
  bool altimeter_distance_valid;

  // Whether or not the altimeter quality is valid.
  bool altimeter_quality_valid;

  // Whether or not the pressure is valid.
  bool pressure_valid;

  // Whether or not the temperature is valid.
  bool temperature_valid;
};

struct FieldCalibrationReport : CommonData
{
  // Hard iron x calibration value (Gauss).
  float hard_iron_x;

  // Hard iron y calibration value (Gauss).
  float hard_iron_y;

  // Hard iron z calibration value (Gauss).
  float hard_iron_z;

  // Soft iron compensation matrix.
  Eigen::Matrix3f soft_iron_matrix;

  // Figure of merit of the calibration.
  float fom;
};

struct FastPressureReport : CommonData
{
  // Pressure (Bar).
  float pressure;
};

struct TransducerReport
{
  // Beam velocity (m/s).
  float velocity;

  // Beam distance (m).
  float distance;

  // Beam uncertainty (m).
  float std;

  // Time (s) from the center of the echo of the cell to the time indicated by timestamp.
  float time_delta;

  // Processed pulse length (s).
  float time_velocity_estimate;

  // Beam velocity is valid.
  bool velocity_valid;

  // Beam distance is valid.
  bool distance_valid;

  // Beam uncertainty is valid.
  bool std_valid;
};

struct VelocityReport : CommonData
{
  // Serial number of the instrument.
  std::uint32_t serial_number;

  // Measured or configured speed of sound (m/s).
  float speed_of_sound;

  // Water temperature (deg C).
  float temperature;

  // Pressure (Bar).
  float pressure;

  // Transducer reports.
  std::array<TransducerReport, 3> transducer_reports;

  // Bottom track x velocity (m/s).
  float vx;

  // Bottom track y velocity (m/s).
  float vy;

  // Bottom track z velocity (m/s).
  float vz;

  // Bottom track velocity covariance matrix.
  Eigen::Matrix3f covariance;

  // All bottom track velocity measurements are valid.
  bool velocity_valid;

  // Bottom track velocity covariance matrix is valid.
  bool covariance_valid;
};

namespace protocol
{

/// Removed the specified number of bytes from the front of the data vector.
auto erase(std::vector<std::uint8_t> & data, std::size_t num_bytes) -> void
{
  if (data.size() < num_bytes) {
    throw std::invalid_argument("Cannot remove more bytes than are present in the data.");
  }
  data.erase(data.begin(), data.begin() + num_bytes);
}

/// Pop and return a value of the specified type from the front of the data vector.
template <typename T>
[[nodiscard]] auto pop(std::vector<std::uint8_t> & data) -> T
{
  const std::size_t bytes = sizeof(T);
  if (data.size() != bytes) {
    throw std::invalid_argument("Cannot deserialize data into the requested type due to mismatched sizes.");
  }

  // this isn't efficient, but the data rates are low enough that it shouldn't matter
  auto popped = data | std::views::take(bytes);
  erase(data, bytes);

  return *reinterpret_cast<const T *>(popped.data());
}

/// Unpack the common data fields from the beginning of a data vector.
auto unpack_common_data(std::vector<std::uint8_t> data, CommonData & common_data)
{
  // unpack the version and data offset
  common_data.version = pop<std::uint8_t>(data);
  common_data.data_offset = pop<std::uint8_t>(data);

  // unpack the flags - the first bit indicates whether POSIX time is used
  const std::uint8_t flags = pop<std::uint8_t>(data);
  common_data.posix_time = (flags & 0x01) != 0;

  // skip the spare byte at this point
  erase(data, 1);

  // unpack the timestamp
  common_data.timestamp = std::chrono::nanoseconds(pop<std::uint32_t>(data));

  // unpack the time since last timestamp
  const auto time_since_stamp = std::chrono::microseconds(pop<std::uint32_t>(data));
  common_data.time_since_stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(time_since_stamp);
}

/// Split the data vector at the given offset.
auto split_data(const std::vector<std::uint8_t> & data, std::uint8_t offset)
  -> std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>
{
  if (data.size() < offset) {
    throw std::invalid_argument("Data size is smaller than the given offset.");
  }

  auto before_offset = data | std::views::take(offset);
  auto after_offset = data | std::views::drop(offset);

  return {{before_offset.begin(), before_offset.end()}, {after_offset.begin(), after_offset.end()}};
}

/// Check the value of the flag at the provided bit.
auto check_flag(std::uint32_t status, std::size_t bit) -> bool { return (status & (1U << bit)) != 0; }

}  // namespace protocol

template <>
struct Deserializer<AHRSReport>
{
  static void from_data(std::vector<std::uint8_t> data, AHRSReport & report)
  {
    protocol::unpack_common_data(data, report);

    // split the data at the offset point
    auto before_after = protocol::split_data(data, report.data_offset);
    auto before = std::move(before_after.first);
    auto after = std::move(before_after.second);

    protocol::erase(before, 16);  // AHRS data starts after 16 bytes

    // deserialize everything up to the offset
    report.serial_number = protocol::pop<std::uint32_t>(before);
    report.operation_mode = protocol::pop<std::uint8_t>(before);

    protocol::erase(before, 3);  // spare bytes

    report.fom = protocol::pop<float>(before);
    report.fom_field_calibration = protocol::pop<float>(before);

    // deserialize everything after the offset
    protocol::erase(after, 3 * sizeof(float));  // roll, pitch, yaw

    const float w = protocol::pop<float>(after);
    const float x = protocol::pop<float>(after);
    const float y = protocol::pop<float>(after);
    const float z = protocol::pop<float>(after);
    report.orientation = Eigen::Quaternionf(w, x, y, z);

    protocol::erase(after, 9 * sizeof(float));  // rotation matrix

    report.declination = protocol::pop<float>(after);
    report.depth = protocol::pop<float>(after);
  }
};

template <>
struct Deserializer<INSReport>
{
  static void from_data(std::vector<std::uint8_t> data, INSReport & report)
  {
    // this modifies a copy of the data
    // we pass a copy to the method, so no slicing occurs
    Deserializer<AHRSReport>::from_data(data, report);

    auto before_after = protocol::split_data(data, report.data_offset);
    auto after = std::move(before_after.second);

    protocol::erase(after, 72);  // drop the AHRS data that was already parsed

    report.fom = protocol::pop<float>(after);

    report.lat_lon_valid = protocol::check_flag(protocol::pop<std::uint32_t>(after), 0);

    report.course_over_ground = protocol::pop<float>(after);
    report.temperature = protocol::pop<float>(after);
    report.pressure = protocol::pop<float>(after);
    report.altitude = protocol::pop<float>(after);
    report.latitude = protocol::pop<double>(after);
    report.longitude = protocol::pop<double>(after);

    protocol::erase(after, sizeof(double));  // reserved bytes

    report.x = protocol::pop<float>(after);
    report.y = protocol::pop<float>(after);
    report.z = protocol::pop<float>(after);

    protocol::erase(after, 3 * sizeof(float));  // vx, vy, vz in NED frame

    report.vx = protocol::pop<float>(after);
    report.vy = protocol::pop<float>(after);
    report.vz = protocol::pop<float>(after);
    report.ground_speed = protocol::pop<float>(after);
    report.wx = protocol::pop<float>(after);
    report.wy = protocol::pop<float>(after);
    report.wz = protocol::pop<float>(after);
  }
};

template <>
struct Deserializer<IMUReport>
{
  static void from_data(std::vector<std::uint8_t> data, IMUReport & report)
  {
    protocol::unpack_common_data(data, report);

    auto before_after = protocol::split_data(data, report.data_offset);
    auto before = std::move(before_after.first);
    auto after = std::move(before_after.second);

    protocol::erase(before, 12);  // IMU data starts after 12 bytes

    // deserialize everything before the offset
    report.imu_valid = protocol::check_flag(protocol::pop<std::uint32_t>(before), 0);

    // deserialize everything after the offset
    report.ax = protocol::pop<float>(after);
    report.ay = protocol::pop<float>(after);
    report.az = protocol::pop<float>(after);
    report.wx = protocol::pop<float>(after);
    report.wy = protocol::pop<float>(after);
    report.wz = protocol::pop<float>(after);
    report.temperature = protocol::pop<float>(after);
  }
};

template <>
struct Deserializer<MagnetometerReport>
{
  static void from_data(std::vector<std::uint8_t> data, MagnetometerReport & report)
  {
    protocol::unpack_common_data(data, report);

    auto before_after = protocol::split_data(data, report.data_offset);
    auto before = std::move(before_after.first);
    auto after = std::move(before_after.second);

    protocol::erase(before, 12);  // IMU data starts after 12 bytes

    // deserialize everything before the offset
    report.is_compensated_for_hard_iron = protocol::check_flag(protocol::pop<std::uint32_t>(after), 0);

    // deserialize everything after the offset
    report.mx = protocol::pop<float>(after);
    report.my = protocol::pop<float>(after);
    report.mz = protocol::pop<float>(after);
  }
};

template <>
struct Deserializer<AltimeterReport>
{
  static void from_data(std::vector<std::uint8_t> data, AltimeterReport & report)
  {
    protocol::unpack_common_data(data, report);

    protocol::erase(data, 12);  // altimeter data starts after 12 bytes

    // this report doesn't have an offset
    const auto flags = protocol::pop<std::uint32_t>(data);
    report.altimeter_distance_valid = protocol::check_flag(flags, 0);
    report.altimeter_quality_valid = protocol::check_flag(flags, 1);
    report.pressure_valid = protocol::check_flag(flags, 16);
    report.temperature_valid = protocol::check_flag(flags, 17);

    report.serial_number = protocol::pop<std::uint32_t>(data);

    protocol::erase(data, 4);  // wtf are they doing with these spare bytes smh...

    report.speed_of_sound = protocol::pop<float>(data);
    report.temperature = protocol::pop<float>(data);
    report.pressure = protocol::pop<float>(data);
    report.distance = protocol::pop<float>(data);
  }
};

template <>
struct Deserializer<FieldCalibrationReport>
{
  static void from_data(std::vector<std::uint8_t> data, FieldCalibrationReport & report)
  {
    protocol::unpack_common_data(data, report);
    protocol::erase(data, report.data_offset);  // field calibration data starts at offset
    report.hard_iron_x = protocol::pop<float>(data);
    report.hard_iron_y = protocol::pop<float>(data);
    report.hard_iron_z = protocol::pop<float>(data);

    report.soft_iron_matrix(0, 0) = protocol::pop<float>(data);
    report.soft_iron_matrix(0, 1) = protocol::pop<float>(data);
    report.soft_iron_matrix(0, 2) = protocol::pop<float>(data);
    report.soft_iron_matrix(1, 0) = protocol::pop<float>(data);
    report.soft_iron_matrix(1, 1) = protocol::pop<float>(data);
    report.soft_iron_matrix(1, 2) = protocol::pop<float>(data);
    report.soft_iron_matrix(2, 0) = protocol::pop<float>(data);
    report.soft_iron_matrix(2, 1) = protocol::pop<float>(data);
    report.soft_iron_matrix(2, 2) = protocol::pop<float>(data);

    protocol::erase(data, 3 * sizeof(float));  // spare bytes

    report.fom = protocol::pop<float>(data);
  }
};

template <>
struct Deserializer<FastPressureReport>
{
  static void from_data(std::vector<std::uint8_t> data, FastPressureReport & report)
  {
    protocol::unpack_common_data(data, report);
    protocol::erase(data, report.data_offset);  // fast pressure data starts at offset
    report.pressure = protocol::pop<float>(data);
  }
};

template <>
struct Deserializer<VelocityReport>
{
  static void from_data(std::vector<std::uint8_t> data, VelocityReport & report)
  {
    protocol::unpack_common_data(data, report);

    // there is no offset for this report type
    protocol::erase(data, 12);  // velocity data starts after 12 bytes

    const auto flags = protocol::pop<std::uint32_t>(data);
    report.transducer_reports[0].velocity_valid = protocol::check_flag(flags, 0);
    report.transducer_reports[1].velocity_valid = protocol::check_flag(flags, 1);
    report.transducer_reports[2].velocity_valid = protocol::check_flag(flags, 2);
    report.transducer_reports[0].distance_valid = protocol::check_flag(flags, 3);
    report.transducer_reports[1].distance_valid = protocol::check_flag(flags, 4);
    report.transducer_reports[2].distance_valid = protocol::check_flag(flags, 5);
    report.transducer_reports[0].std_valid = protocol::check_flag(flags, 6);
    report.transducer_reports[1].std_valid = protocol::check_flag(flags, 7);
    report.transducer_reports[2].std_valid = protocol::check_flag(flags, 8);

    bool velocity_x_valid = protocol::check_flag(flags, 9);
    bool velocity_y_valid = protocol::check_flag(flags, 10);
    bool velocity_z_valid = protocol::check_flag(flags, 11);
    report.velocity_valid = velocity_x_valid && velocity_y_valid && velocity_z_valid;

    bool covariance_xx_valid = protocol::check_flag(flags, 12);
    bool covariance_yy_valid = protocol::check_flag(flags, 13);
    bool covariance_zz_valid = protocol::check_flag(flags, 14);
    report.covariance_valid = covariance_xx_valid && covariance_yy_valid && covariance_zz_valid;

    report.serial_number = protocol::pop<std::uint32_t>(data);

    protocol::erase(data, 4);  // spare bytes

    report.speed_of_sound = protocol::pop<float>(data);
    report.temperature = protocol::pop<float>(data);
    report.pressure = protocol::pop<float>(data);

    report.transducer_reports[0].velocity = protocol::pop<float>(data);
    report.transducer_reports[1].velocity = protocol::pop<float>(data);
    report.transducer_reports[2].velocity = protocol::pop<float>(data);

    report.transducer_reports[0].distance = protocol::pop<float>(data);
    report.transducer_reports[1].distance = protocol::pop<float>(data);
    report.transducer_reports[2].distance = protocol::pop<float>(data);

    report.transducer_reports[0].std = protocol::pop<float>(data);
    report.transducer_reports[1].std = protocol::pop<float>(data);
    report.transducer_reports[2].std = protocol::pop<float>(data);

    report.transducer_reports[0].time_delta = protocol::pop<float>(data);
    report.transducer_reports[1].time_delta = protocol::pop<float>(data);
    report.transducer_reports[2].time_delta = protocol::pop<float>(data);

    report.transducer_reports[0].time_velocity_estimate = protocol::pop<float>(data);
    report.transducer_reports[1].time_velocity_estimate = protocol::pop<float>(data);
    report.transducer_reports[2].time_velocity_estimate = protocol::pop<float>(data);

    report.vx = protocol::pop<float>(data);
    report.vy = protocol::pop<float>(data);
    report.vz = protocol::pop<float>(data);

    report.covariance(0, 0) = protocol::pop<float>(data);
    report.covariance(1, 1) = protocol::pop<float>(data);
    report.covariance(2, 2) = protocol::pop<float>(data);
  }
};

template <>
struct Deserializer<std::string>
{
  static void from_data(std::vector<std::uint8_t> data, std::string & response)
  {
    response = std::string(data.begin(), data.end());
  }
};

}  // namespace nucleus
