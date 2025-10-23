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
#include <cstdint>

namespace nucleus
{

struct AHRSReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

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

struct INSReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

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

  // Relative x position (m).
  float x;

  // Relative y position (m).
  float y;

  // Relative z position (m).
  float z;

  // Velocity along the sensor x axis (m/s).
  float vx;

  // Velocity along the sensor y axis (m/s).
  float vy;

  // Velocity along the sensor z axis (m/s).
  float vz;

  // Angular velocity about the sensor x axis (rad/s).
  float wx;

  // Angular velocity about the sensor y axis (rad/s).
  float wy;

  // Angular velocity about the sensor z axis (rad/s).
  float wz;
};

struct IMUReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

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
};

struct MagnetometerReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

  // Magnetic field along the x axis (gauss).
  float mx;

  // Magnetic field along the y axis (gauss).
  float my;

  // Magnetic field along the z axis (gauss).
  float mz;
};

struct AltimeterReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

  // Instrument serial number.
  std::uint32_t serial_number;

  // Measured or configured speed of sound (m/s).
  float speed_of_sound;

  // Altitude (m).
  float altitude;

  // Pressure (Bar).
  float pressure;

  // Temperature (deg C).
  float temperature;
};

struct FieldCalibrationReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

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

struct FastPressureReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

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

struct VelocityReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

  // Serial number of the instrument.
  std::uint32_t serial_number;

  // Measured or configured speed of sound (m/s).
  float speed_of_sound;

  // Water temperature (deg C).
  float temperature;

  // Pressure (Bar).
  float pressure;

  // Transducer reports.
  std::array<TransducerReport, 3> transducers;

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

struct CurrentProfileReport
{
  // Timestamp of the report (nanoseconds since epoch).
  std::chrono::nanoseconds timestamp;

  // Serial number of the instrument.
  std::uint32_t serial_number;

  // Speed of sound (m/s).
  float speed_of_sound;

  // Water temperature (deg C).
  float temperature;

  // Pressure (Bar).
  float pressure;

  // Cell size (m).
  float cell_size;

  // Blanking distance (m).
  float blanking_distance;

  // Number of cells in the current profile data.
  //
  // This value determines the dimensions of the velocity amplitude and correlation data.
  std::uint16_t number_of_cells;
};

struct CommandResponse
{
  /// Whether or not the command was successful.
  bool success;

  /// The error message, if any.
  std::string error_message;

  /// The error code, if any.
  int error_code;

  /// The valid limits for the command, if an error was experienced.
  std::string valid_limits;
};

namespace protocol
{

struct adl_serializer<AHRSReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, AHRSReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<INSReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, INSReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<IMUReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, IMUReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<MagnetometerReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, MagnetometerReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<AltimeterReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, AltimeterReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<FieldCalibrationReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, FieldCalibrationReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<FastPressureReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, FastPressureReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<VelocityReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, VelocityReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<CurrentProfileReport>
{
  static void from_data(const std::vector<std::uint8_t> & data, CurrentProfileReport & report)
  {
    // TODO: deserialize
  }
};

struct adl_serializer<CommandResponse>
{
  static void from_data(const std::vector<std::uint8_t> & data, CommandResponse & response)
  {
    // TODO: deserialize
  }
};

}  // namespace protocol

}  // namespace nucleus
